$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/handshake-capture-test"
New-Item -ItemType Directory -Force $out | Out-Null
$server="$out/server.log";$client="$out/client.log"
'old WARN  [network] WorldSocket initialization timed out' | Set-Content $server
'fixture client before restart' | Set-Content $client
$tool="$repo/tools/research/Save-WorldHandshakeIncident.ps1"
$result=& $tool -ServerLog $server -ClientLog $client -OutputDirectory $out -Watch -WatchSeconds 1
if($result -ne 'No new handshake timeout during the observation window.'){throw 'Old timeout triggered a new incident'}
$job=Start-Job -ArgumentList $server -ScriptBlock {
    param($path)
    Start-Sleep -Milliseconds 500
    [IO.File]::AppendAllText($path,"new WARN  [network] WorldSocket initialization timed")
    Start-Sleep -Milliseconds 300
    [IO.File]::AppendAllText($path," out after 10 seconds`n")
}
try{
    $result=& $tool -ServerLog $server -ClientLog $client -OutputDirectory $out -Watch -WatchSeconds 15
    if(!(Test-Path -LiteralPath "$result/Client.log")){throw 'New fragmented timeout was not saved'}
    if((Get-Content "$result/Client.log" -Raw).Trim() -ne 'fixture client before restart'){throw 'Client snapshot differs'}
    if((Get-Content "$result/server-network.log" -Raw) -notmatch 'new WARN'){throw 'Incident omitted new timeout'}
    $history=Get-Content "$result/tcp-history.json" -Raw | ConvertFrom-Json
    if(!@($history).Count -or !$history[0].CapturedUtc){throw 'No pre-abort TCP observation was preserved'}
}finally{Wait-Job $job | Out-Null;Receive-Job $job | Out-Null;Remove-Job $job}
'PASS: old records ignored; fragmented new timeout preserves client and correlated network evidence.'

# Only the identified failure on the instance listener should freeze the
# thread trace. A successful first login must leave it armed for the next one.
$deadlineLine='INFO  [network] World handshake deadline expired: 127.0.0.1:54321 -> local 8086, age 10007 ms, type 0, initializer 0, continued auth 0, buffered 0; TCP info 1 error 0 state 4 age 10007 ms sent 48 received 0 unacked 0 retransmitted 0 send window 65280 receive window 65535'
Set-Content -LiteralPath $server -Value $deadlineLine
$job=Start-Job -ArgumentList $server,$deadlineLine -ScriptBlock {
    param($path,$line)
    Start-Sleep -Milliseconds 800
    $otherPort=$line.Replace('local 8086','local 8085')
    $unacked=$line.Replace('unacked 0','unacked 48')
    $success=$line.Replace('deadline expired','encryption acknowledged')
    [IO.File]::AppendAllText($path,"$otherPort`n$unacked`n$success`n")
}
try {
    $result=& $tool -ServerLog $server -ClientLog $client -OutputDirectory $out -Watch -SilentInstancePort 8086 -WatchSeconds 4
    if ($result -ne 'No new handshake timeout during the observation window.') { throw 'Unrelated handshake triggered the instance recorder' }
} finally { Wait-Job $job | Out-Null; Receive-Job $job | Out-Null; Remove-Job $job }
$job=Start-Job -ArgumentList $server,$deadlineLine -ScriptBlock {
    param($path,$line)
    Start-Sleep -Milliseconds 800
    [IO.File]::AppendAllText($path,$line.Substring(0,100))
    Start-Sleep -Milliseconds 350
    [IO.File]::AppendAllText($path,$line.Substring(100) + "`n")
}
try {
    $freeze={
        param($incident)
        if (Test-Path -LiteralPath "$incident/tcp-current.json") { throw 'Trace was frozen after TCP inspection' }
        'triggered' | Set-Content -LiteralPath "$incident/trace-trigger.txt"
    }
    $result=& $tool -ServerLog $server -ClientLog $client -OutputDirectory $out -Watch -SilentInstancePort 8086 -WatchSeconds 12 -OnIncident $freeze
    if (!(Test-Path -LiteralPath "$result/server-network.log")) { throw 'Silent instance deadline did not trigger the recorder' }
    if (!(Test-Path -LiteralPath "$result/trace-trigger.txt")) { throw 'The matching deadline did not freeze the trace' }
} finally { Wait-Job $job | Out-Null; Receive-Job $job | Out-Null; Remove-Job $job }
'PASS: only a newly appended silent instance deadline triggers the failure-specific watch.'
