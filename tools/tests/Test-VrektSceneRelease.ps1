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
# A stale scene has all the same calls. Check the actual coordinate arithmetic
# and its stack load/store order, not just source timestamps or symbol presence.
$range=Bounds '?MovementInform@npc_vrekt_captured_drake@@'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dump=Get-ChildItem "$vs/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe" | Select-Object -Last 1
if(!$dump){throw 'Native MSVC disassembler unavailable'}
$listing=(& $dump.FullName /DISASM:NOBYTES ('/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]) $exe) -join "`n"
if($LASTEXITCODE){throw 'Native bounded escape disassembly failed'}
$operations=@([regex]::Matches($listing,'\b(addss|subss)\s+(xmm\d+),dword ptr \[([0-9A-Fa-f]+)h\]'))
if($operations.Count -ne 3){throw 'Re-audit compiled escape coordinate arithmetic'}
$loads=@();$stores=@()
for($axis=0;$axis -lt 3;$axis++){
 $operation=$operations[$axis];$register=$operation.Groups[2].Value
 $constant=[BitConverter]::ToSingle($binary,(Offset ([Convert]::ToInt64($operation.Groups[3].Value,16))))
 if($operation.Groups[1].Value -ne @('subss','addss','addss')[$axis] -or $constant -ne @(45.0,60.0,60.0)[$axis]){throw 'Deployed escape is not home(-45,+60,+60)'}
 $before=$listing.Substring(0,$operation.Index);$after=$listing.Substring($operation.Index+$operation.Length)
 $load=@([regex]::Matches($before,('\bmovss\s+'+$register+',dword ptr \[rsp\+([0-9A-Fa-f]+)h\]')))
 $save=[regex]::Match($after,('\bmovss\s+dword ptr \[rsp\+([0-9A-Fa-f]+)h\],'+$register+'\b'))
 if(!$load.Count -or !$save.Success){throw 'Missing coordinate load/store dataflow'}
 $loads+=[Convert]::ToInt32($load[-1].Groups[1].Value,16);$stores+=[Convert]::ToInt32($save.Groups[1].Value,16)
}
for($axis=1;$axis -lt 3;$axis++){
 if($loads[$axis] -ne $loads[0]+4*$axis -or $stores[$axis] -ne $stores[0]+4*$axis){throw 'Escape arithmetic does not map to consecutive XYZ coordinates'}
}
$output="$repo/build-extractors/runtime-test/vrekt-scene"
New-Item -ItemType Directory -Force $output | Out-Null
$listing | Set-Content "$output/escape-disassembly.txt"
'PASS: canonical escape XYZ loads, float constants and destination stores implement home(-45,+60,+60), including the repaired positive-Y direction.'
'PASS: actual final-link scene library freshness, native summon/takeoff/escape and inherited combat/charm paths are present in the canonical Release. Client visuals/travel remain pending.'
