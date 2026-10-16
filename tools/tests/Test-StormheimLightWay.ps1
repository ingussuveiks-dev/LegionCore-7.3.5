param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimLightWayData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimSupplies.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$PSScriptRoot/../../.codex/light-way-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-light-way"
$previous="$repo/build-extractors/runtime-test/stormheim-helheim-supplies"
Get-ChildItem "$previous/*.inc" | Copy-Item -Destination $output
$fixture=Get-Content "$previous/HelheimSuppliesFixture.inc" -Raw
$fixture=$fixture.Replace('struct CreatureTemplate {','struct CreatureTemplate {uint32 Entry=91657,QuestItem[6]{128854},KillCredit[2]{},lootid=91657;bool QuestPersonalLoot=true;')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {bool HasQuestForCreature(CreatureTemplate const*)const;')
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {bool IsPersonalForQuest(Player const*)const;bool IsDamageEnoughForLootingAndReward(){return true;}void SetLootRecipient(Unit*){}')
$fixture=$fixture.Replace('CreatureTemplate const* GetCreatureTemplate(){','CreatureTemplate const* GetCreatureTemplate()const{')
[IO.File]::WriteAllText("$output/HelheimSuppliesFixture.inc",$fixture)
$source=Get-Content "$PSScriptRoot/StormheimHelheimSuppliesTest.cpp" -Raw
[IO.File]::WriteAllText("$output/LightWaySupport.inc",$source.Substring(0,$source.IndexOf('int main(')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('bool Player::HasQuestForCreature(');$b=$core.IndexOf('int32 Player::GetQuestObjectiveData(',$a)
[IO.File]::WriteAllText("$output/HasQuestForCreature.inc",$core.Substring($a,$b-$a))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$a=$creature.IndexOf('bool Creature::IsPersonalForQuest(');$b=$creature.IndexOf('bool Creature::IsEventCreature(',$a)
[IO.File]::WriteAllText("$output/PersonalQuest.inc",$creature.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('    // find player: owner of controlled', $unit.IndexOf('void Unit::Kill('));$b=$unit.IndexOf('    // Reward player, his pets, and group/raid members',$a)
[IO.File]::WriteAllText("$output/KillOwner.inc",$unit.Substring($a,$b-$a))
$a=$unit.IndexOf('    uint32 lootid =', $unit.IndexOf('void Unit::GenerateLoot('));$b=$unit.IndexOf('    Group* group',$a)
[IO.File]::WriteAllText("$output/CreatureLootSelector.inc",$unit.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimLightWayTest.cpp`" /Fe`"$output/StormheimLightWay.exe`" /Fo`"$output/StormheimLightWay.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimLightWay.exe`" `"$output/drop.tsv`"`r`n"
[IO.File]::WriteAllText("$output/run-StormheimLightWay.cmd",$cmd)
& "$output/run-StormheimLightWay.cmd"
if($LASTEXITCODE){throw 'LightWay production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe"
 $bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('Quest reward: next quest %u is not available after reward of %u.')){throw 'Canonical Release lacks reward-next admission implementation'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Actual final-link game input missing'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical executable predates actual game final-link input'}
 'PASS: canonical executable reward-next diagnostic marker and actual game final-link input.'
}
