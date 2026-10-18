param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boss-quest-loot"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$a,[string]$b){$start=$s.IndexOf($a);if($start -lt 0){throw "Missing $a"};$end=$s.IndexOf($b,$start);if($end -le $start){throw "Missing $b"};return $s.Substring($start,$end-$start)}
$path='src/server/game/Loot/LootMgr.cpp'
$source=Get-Content "$repo/$path" -Raw
[IO.File]::WriteAllText("$out/BossLoot.inc",(Extract $source 'void LootTemplate::ProcessBossLoot(' 'void LootTemplate::ProcessRareOrGoLoot('))
[IO.File]::WriteAllText("$out/BossRowRoll.inc",(Extract $source 'bool LootStoreItem::Roll(' 'bool LootStoreItem::IsValid('))
$old=(& git -C $repo show "7653a4ec:$path") -join "`n"
if($LASTEXITCODE){throw 'Missing pinned boss-loot baseline7653a4ec'}
[IO.File]::WriteAllText("$out/OldBossLoot.inc",(Extract $old 'void LootTemplate::ProcessBossLoot(' 'void LootTemplate::ProcessRareOrGoLoot(').Replace('LootTemplate::ProcessBossLoot','LootTemplate::OldProcessBossLoot'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/BossQuestLootTest.cpp`" /Fe`"$out/Odyn.exe`" /Fo`"$out/Odyn.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Odyn.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native boss quest-loot regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('quest %u gearRoll %u')){throw 'Canonical Release lacks boss quest/gear diagnostic'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual game final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates game library'}
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 function Symbol([string]$name){
  $index=$map.IndexOf(' '+$name);if($index -lt 0){throw "Missing canonical symbol: $name"}
  $line=$map.Substring($index,$map.IndexOf("`n",$index)-$index)
  $m=[regex]::Match($line,'\s([0-9A-Fa-f]{16})\s+f')
  if(!$m.Success){throw "Missing canonical symbol address: $name"};[Convert]::ToInt64($m.Groups[1].Value,16)
 }
 $binary=[IO.File]::ReadAllBytes($exe);$pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24);$sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function Offset([long]$address){
  $relative=$address-$imageBase
  for($i=0;$i -lt [BitConverter]::ToUInt16($binary,$pe+6);$i++){
   $section=$sections+40*$i;$start=[BitConverter]::ToUInt32($binary,$section+12)
   $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
   if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$relative-$start)}
  };throw 'Address outside canonical PE'
 }
 function Resolve([long]$address){
  for($i=0;$i -lt 10;$i++){$at=Offset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
  throw 'Unexpected incremental-link thunk cycle'
 }
 $name='?ProcessBossLoot@LootTemplate@@'
 $address=Resolve (Symbol $name);$at=Offset $address;$found=$false
 $marker='quest %u gearRoll %u'
 for($i=0;$i -lt 4090;$i++){
  if($binary[$at+$i] -in @(0x48,0x4c) -and $binary[$at+$i+1] -eq 0x8d -and ($binary[$at+$i+2] -band 0xc7) -eq 5){
   try{$target=Offset ($address+$i+7+[BitConverter]::ToInt32($binary,$at+$i+3));$text=[Text.Encoding]::ASCII.GetString($binary,$target,150);if($text.Split([char]0)[0].Contains($marker)){$found=$true;break}}catch{}
  }
 }
 if(!$found){throw 'Canonical boss processor does not reference the quest/gear-roll diagnostic'}
 'PASS: actual game library and canonical boss processor reference the new quest/gear-roll path (PE/thunks resolved).'
}
