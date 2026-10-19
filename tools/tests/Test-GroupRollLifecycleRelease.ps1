$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RollCompletionRelease.ps1"
foreach($path in @('src/server/game/Groups/Group.h','src/server/game/Server/WorldSession.cpp')){
 if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Final-link game library predates $path"}
}
$helper=Resolve (Symbol '?RemoveMemberFromRolls@Group@@')
$finish=Resolve (Symbol '?CountTheRoll@Group@@')
if(@(Calls '?RemoveMemberQueue@Group@@') -notcontains $helper){throw 'Canonical native removal bypasses roll participant cleanup'}
$cleanup=@(Calls '?RemoveMemberFromRolls@Group@@')
if($cleanup -notcontains $finish -or $cleanup -contains (Resolve (Symbol '?CountRollVote@Group@@'))){throw 'Canonical cleanup still relies on a removed voter or lacks direct completion'}
if(@(Calls '?Disband@Group@@') -notcontains $finish){throw 'Canonical disband does not settle/cancel live rolls'}
$logoutCalls=@(Calls '?LogoutPlayer@WorldSession@@') | Where-Object {$_ -eq $helper}
if($logoutCalls.Count -lt 1 -or $logoutCalls.Count -gt 2){throw 'Canonical logout lacks current/original party roll cleanup'}
$out=Join-Path $repo 'build-extractors/runtime-test/group-roll-lifecycle-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;removal_cleanup=$true;direct_resolution=$true;disband_cleanup=$true;logout_cleanup_calls=$logoutCalls.Count} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release removal, direct roll resolution, disband and current/original-party logout cleanup verified through actual PE bounds/thunks and final-link freshness.'
