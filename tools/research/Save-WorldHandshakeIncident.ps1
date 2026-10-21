param(
    [string]$ServerLog,
    [Parameter(Mandatory=$true)][string]$ClientLog,
    [string]$OutputDirectory,
    [switch]$Watch,
    [ValidateRange(1,86400)][int]$WatchSeconds=3600
)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$ServerLog){$ServerLog="$repo/build-extractors/bin/Release/logs/Server.log"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/handshake-incidents"}
$history=[Collections.Generic.Queue[object]]::new()

function Read-ConnectionSnapshot {
    $processes=@(Get-CimInstance Win32_Process | Where-Object {$_.Name -match '^(Wow.*|worldserver|bnetserver)\.exe$'} | Select-Object ProcessId,Name,ExecutablePath)
    $ids=@($processes | ForEach-Object {$_.ProcessId})
    $connections=@(if($ids.Count){
        Get-NetTCPConnection -ErrorAction SilentlyContinue | Where-Object {$ids -contains $_.OwningProcess} |
            Select-Object OwningProcess,State,LocalAddress,LocalPort,RemoteAddress,RemotePort
    })
    [pscustomobject]@{CapturedUtc=[DateTime]::UtcNow.ToString('o');Processes=$processes;Connections=$connections}
}

function Save-Incident {
    # Output is deliberately private runtime data, never part of the commit.
    $destination=Join-Path $OutputDirectory ([DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Force $destination | Out-Null
    $network=@(Get-Content -LiteralPath $ServerLog -Tail 2500 | Where-Object {
        $_ -match '\[network\]' -and $_ -match 'handshake|initialization timed out|instance connection|instance serial|CONNECT_TO_FAILED|LogDisconnect'
    })
    $network | Set-Content -LiteralPath "$destination/server-network.log" -Encoding UTF8
    if(Test-Path -LiteralPath $ClientLog){Copy-Item -LiteralPath $ClientLog -Destination "$destination/Client.log"}
    $snapshot=Read-ConnectionSnapshot
    $snapshot | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$destination/tcp-current.json" -Encoding UTF8
    # The deadline abort closes the failed socket before a later query can
    # identify its owner. Preserve observations from BEFORE that abort as well.
    ConvertTo-Json -InputObject @($history.ToArray()) -Depth 6 | Set-Content -LiteralPath "$destination/tcp-history.json" -Encoding UTF8
    Write-Output $destination
}

if(!$Watch){Save-Incident;return}
# Follow only newly appended records, including a log replaced on server start.
# No packet contents, process memory, settings or server/client state are changed.
$position=if(Test-Path -LiteralPath $ServerLog){(Get-Item -LiteralPath $ServerLog).Length}else{0}
$identity=if(Test-Path -LiteralPath $ServerLog){(Get-Item -LiteralPath $ServerLog).CreationTimeUtc.Ticks}else{0}
$deadline=[DateTime]::UtcNow.AddSeconds($WatchSeconds)
$nextSnapshot=[DateTime]::MinValue
$pending=''
while([DateTime]::UtcNow -lt $deadline){
    if([DateTime]::UtcNow -ge $nextSnapshot){
        $history.Enqueue((Read-ConnectionSnapshot))
        while($history.Count -gt 30){[void]$history.Dequeue()}
        $nextSnapshot=[DateTime]::UtcNow.AddSeconds(1)
    }
    if(Test-Path -LiteralPath $ServerLog){
        $file=Get-Item -LiteralPath $ServerLog
        if($file.Length -lt $position -or $file.CreationTimeUtc.Ticks -ne $identity){$position=0;$pending='';$identity=$file.CreationTimeUtc.Ticks}
        $stream=[IO.File]::Open($ServerLog,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
        try{
            [void]$stream.Seek($position,[IO.SeekOrigin]::Begin)
            $reader=[IO.StreamReader]::new($stream)
            $pending+=$reader.ReadToEnd()
            $position=$stream.Position
        }finally{$stream.Dispose()}
        $lines=$pending -split "`n"
        $pending=$lines[-1]
        if($lines.Count -gt 1 -and ($lines[0..($lines.Count-2)] -match '\bWARN\s+\[network\].*WorldSocket initialization timed out')){
            Save-Incident
            return
        }
    }
    Start-Sleep -Milliseconds 200
}
Write-Output 'No new handshake timeout during the observation window.'
