$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-VrektNpcRelease.ps1"
$scriptsRelative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
$sceneLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $scriptsRelative))
foreach($path in @('src/server/scripts/Legion/zone_stormheim.cpp','src/server/scripts/Legion/vrekt_scene.h')){
 if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $sceneLib).LastWriteTimeUtc){throw "Final-link scripts predates $path"}
}
function Overloads([string]$prefix){
 $start=0
 while(($at=$map.IndexOf(' '+$prefix,$start)) -ge 0){
  $line=$map.Substring($at,$map.IndexOf("`n",$at)-$at)
  $match=[regex]::Match($line,'\s+([0-9A-Fa-f]{16})\s+f')
  if($match.Success){Resolve ([Convert]::ToInt64($match.Groups[1].Value,16))}
  $start=$at+1
 }
}
foreach($pair in @(
 @('?UpdateLifecycle@npc_vrekt_vaettfang@@','?SummonCreature@WorldObject@@'),
 @('?UpdateLifecycle@npc_vrekt_captured_drake@@','?MoveTakeoff@MotionMaster@@'),
 @('?MovementInform@npc_vrekt_captured_drake@@','?MoveSmoothFlyPath@MotionMaster@@'),
 @('?JustDied@npc_vrekt_vaettfang@@','?JustDied@AggressorAI@@'),
 @('?UpdateAI@npc_vrekt_village_drake@@','?UpdateAI@SmartAI@@'),
 @('?AttackStart@npc_vrekt_village_drake@@','?AttackStart@SmartAI@@'))){
 $targets=@(Overloads $pair[1])
 if(!(@(Calls $pair[0]) | Where-Object {$_ -in $targets})){throw "Canonical scene lacks $($pair[1]) in $($pair[0])"}
}
[void](VirtualSlot '??_7npc_vrekt_vaettfang@@6B@' '?OnCharmed@CreatureAI@@')
[void](VirtualSlot '??_7npc_vrekt_village_drake@@6B@' '?OnCharmed@SmartAI@@')
[void](VirtualSlot '??_7npc_vrekt_captured_drake@@6B@' '?UpdateLifecycle@npc_vrekt_captured_drake@@')
'PASS: actual final-link scene library freshness, native summon/takeoff/escape and inherited combat/charm paths are present in the canonical Release. Client visuals/travel remain pending.'
