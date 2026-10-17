param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-dreadwake"
& "$PSScriptRoot/Test-StormheimDreadwakeData.ps1"
& "$PSScriptRoot/Test-StormheimShields.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-shields/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {bool flight=false,rooted=false,stunned=false,loggingOut=false,controlBlocked=false;unsigned taxiCalls=0,taxiStarts=0;bool ActivateTaxiPathTo(uint32);bool isInFlight(){return flight;}')
$f=$f.Replace('HeightMap* GetMap(){return &heightMap;}','FakeMap* GetMap(){return ShieldsMap();}')
$f=$f.Replace('uint32 GetInstanceId(){return 0;}','uint32 instance=0;uint32 GetInstanceId(){return instance;}')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_dreadwake.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/DreadwakeProduction.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_dreadwake()')))
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$native.IndexOf('bool Player::ActivateTaxiPathTo(std::vector');$b=$native.IndexOf('bool Player::ShortTaxiPathTo(',$a)
[IO.File]::WriteAllText("$out/DreadwakeNativeTaxi.inc",$native.Substring($a,$b-$a).Replace('Player::','TaxiPlayer::'))
$a=$native.IndexOf('bool Player::ActivateTaxiPathTo(uint32');$b=$native.IndexOf('void Player::CleanupAfterTaxiFlight(',$a)
if($b -lt 0){throw 'Native taxi overload extraction boundary missing'}
[IO.File]::WriteAllText("$out/DreadwakeTaxiOverload.inc",$native.Substring($a,$b-$a).Replace('Player::','TaxiPlayer::'))
$native=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$native.IndexOf('void Spell::EffectKillCredit(');$b=$native.IndexOf('void Spell::EffectQuestFail(',$a)
[IO.File]::WriteAllText("$out/DreadwakeNativeCredit.inc",$native.Substring($a,$b-$a))
$native=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$native.IndexOf('        Unit* caster = (itr->second.castFlags');$b=$native.IndexOf('        SpellInfo const* spellEntry',$a)
[IO.File]::WriteAllText("$out/DreadwakeClickActors.inc",$native.Substring($a,$b-$a).Replace('this','catapult'))
$header=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.h" -Raw
$a=$header.IndexOf('enum class GossipOptionNpc');$b=$header.IndexOf('//POI icons',$a);$enums=$header.Substring($a,$b-$a)
$header=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
$a=$header.IndexOf('enum NPCFlags :');$b=$header.IndexOf('enum MovementFlags',$a)
[IO.File]::WriteAllText("$out/DreadwakeGossipEnums.inc",$enums+$header.Substring($a,$b-$a))
$native=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$a=$native.IndexOf('uint64 GossipMenu::GetRequiredNpcFlagForOption');$b=$native.IndexOf('PlayerMenu::PlayerMenu',$a)
[IO.File]::WriteAllText("$out/DreadwakeGossipFlags.inc",$native.Substring($a,$b-$a))
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$admission=[regex]::Match($native,'            if \(!\(GossipMenu::GetRequiredNpcFlagForOption\(optionNpc\) & npcflags\)\)\s*continue;')
if(!$admission.Success){throw 'Native gossip admission extraction missing'}
[IO.File]::WriteAllText("$out/DreadwakeGossipAdmission.inc",$admission.Value)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimDreadwakeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dreadwake native/production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('Dreadwake: Cullen native quest taxi5262','npc_stormheim_dreadwake_cullen')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing scripts final-link dependency'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item "$repo/src/server/scripts/Legion/stormheim_dreadwake.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates Cullen implementation'}
 'PASS: canonical Cullen Release marker/registration and actual scripts.lib final-link input freshness.'
}
