$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialRaidChestRelease.ps1"
foreach($path in @('Entities/Player/Player.cpp','Entities/Player/Player.h','Entities/Object/Object.cpp','Handlers/LootHandler.cpp','Spells/SpellEffects.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Actual final-link game library predates $path"}
}
$pending=Resolve (Symbol '?HasPendingPersonalChestLoot@Player@@')
foreach($name in @('?canSeeOrDetect@WorldObject@@','?EffectOpenLock@Spell@@')){
 if(@(Calls $name) -notcontains $pending){throw "Canonical $name lacks earned-container admission"}
}
if(@(Calls '?HasPendingPersonalChestLoot@Player@@') -notcontains (Resolve (Symbol '?hasItemFor@Loot@@'))){throw 'Canonical pending admission lacks actual loot eligibility'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;visibility_and_opening_use_native_pending_admission=$true;pending_checks_actual_loot_eligibility=$true} | ConvertTo-Json | Set-Content "$out/pending-release-inputs.json"
'PASS: canonical PE/thunks share earned-container admission in visibility and Opening, including actual loot eligibility.'
