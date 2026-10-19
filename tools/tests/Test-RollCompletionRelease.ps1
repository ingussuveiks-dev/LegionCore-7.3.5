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
$completion=@(Calls '?CountTheRoll@Group@@')
foreach($name in @('?GetItemInSlot@Loot@@','?AllowedForPlayer@Loot@@')){
 if($completion -notcontains (Resolve (Symbol $name))){throw "Canonical completion lacks $name"}
}
$range=Bounds '?EndRoll@Group@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$ending=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
if($LASTEXITCODE){throw 'Canonical timeout disassembly failed'}
$ending | Set-Content "$out/endroll-disassembly.txt"
# isValid/getLoot are inlined by this Release. Resolve the real registry call
# followed by its null branch instead of requiring a non-inlined wrapper.
$lookup=Resolve (Symbol '?GetLoot@LootMgr@@');$guarded=$false
foreach($site in [regex]::Matches($ending,'call\s+([0-9A-Fa-f]+)\s*\r?\n[^\r\n]*test\s+rax,rax\s*\r?\n[^\r\n]*je\s+([0-9A-Fa-f]+)')){
 if((Resolve ([Convert]::ToInt64($site.Groups[1].Value,16))) -eq $lookup){$guarded=$true}
}
if(!$guarded -or @(Calls '?EndRoll@Group@@') -notcontains (Resolve (Symbol '?CountTheRoll@Group@@'))){throw 'Canonical timeout lacks native missing-registry cleanup'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;ordinary_quest_slot=$true;single_enabled_vote=$true;winner_current_admission=$true;invalid_registry_cleanup=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release validates actual vote/slot/current winner admission and timeout cleanup; final-link freshness and hashes recorded.'
