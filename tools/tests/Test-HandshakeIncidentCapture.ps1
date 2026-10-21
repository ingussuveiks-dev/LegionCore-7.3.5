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
