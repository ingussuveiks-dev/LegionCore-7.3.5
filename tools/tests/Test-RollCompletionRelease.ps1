$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-MasterLootRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/roll-completion-native'
New-Item -ItemType Directory -Force $out | Out-Null
$voteCalls=@(Calls '?CountRollVote@Group@@')
if($voteCalls -notcontains (Resolve (Symbol '?GetItemInSlot@Loot@@'))){throw 'Canonical vote path lacks ordinary/quest slot admission'}
if($voteCalls -notcontains (Resolve (Symbol '?CountTheRoll@Group@@'))){throw 'Canonical vote path lacks native resolution'}
$range=Bounds '?CountRollVote@Group@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
if($LASTEXITCODE){throw 'Canonical vote disassembly failed'}
$listing | Set-Content "$out/vote-disassembly.txt"
foreach($pattern in @('cmp\s+dword ptr \[[^\]]+\],4\s*\r?\n[^\r\n]*jne','cmp\s+r[a-z0-9]+,4\s*\r?\n[^\r\n]*jae','shl\s+[a-z0-9]+,cl\s*\r?\n[^\r\n]*test\s+byte ptr')){
 if($listing -notmatch $pattern){throw "Canonical roll lacks single/enabled vote admission: $pattern"}
}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;ordinary_quest_slot=$true;single_enabled_vote=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release votes resolve the actual ordinary/quest slot, reject repeated/disabled votes and call native completion; final-link freshness and hashes recorded.'
