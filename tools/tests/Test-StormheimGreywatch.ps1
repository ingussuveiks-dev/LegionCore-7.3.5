param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimGreywatchData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-greywatch"
& "$PSScriptRoot/Test-StormheimBuiltScale.ps1" *> "$out/loot-upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-built-scale/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$fixture=$fixture.Replace('struct SpellInfo {','struct SpellInfo {struct Misc {struct Data{uint32 SchoolMask=1;}MiscData;}misc;Misc* GetMisc()const{return const_cast<Misc*>(&misc);}')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/StormheimBuiltScaleTest.cpp" -Raw
[IO.File]::WriteAllText("$out/GreywatchLootFixture.inc",$prefix.Substring(0,$prefix.IndexOf('int main(')))
$native=Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw
foreach($s in @(@('TorchTarget','SpellCastResult SpellInfo::CheckTarget(','SpellCastResult SpellInfo::CheckExplicitTarget('),@('TorchExplicit','SpellCastResult SpellInfo::CheckExplicitTarget(','bool SpellInfo::CheckTargetCreatureType('),@('TorchCreatureType','bool SpellInfo::CheckTargetCreatureType(','SpellSchoolMask SpellInfo::GetSchoolMask('),@('TorchDeadTarget','bool SpellInfo::IsRequiringDeadTarget()','bool SpellInfo::CanBeUsedInCombat()'))){
 $a=$native.IndexOf($s[1]);$b=$native.IndexOf($s[2],$a);if($a -lt 0 -or $b -le $a){throw "Missing native $($s[0])"};[IO.File]::WriteAllText("$out/$($s[0]).inc",$native.Substring($a,$b-$a))
}
$spells=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spells.IndexOf('bool WorldObjectSpellAreaTargetCheck::operator()');$b=$spells.IndexOf('WorldObjectSpellBetweenTargetCheck::WorldObjectSpellBetweenTargetCheck',$a)
[IO.File]::WriteAllText("$out/TorchArea.inc",$spells.Substring($a,$b-$a))
$defines=Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw
$attributes=[regex]::Matches(($native.Substring($native.IndexOf('SpellCastResult SpellInfo::CheckTarget('),$native.IndexOf('SpellCastResult SpellInfo::CheckExplicitTarget(')-$native.IndexOf('SpellCastResult SpellInfo::CheckTarget('))+' SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS SPELL_ATTR2_CAN_TARGET_DEAD'),'SPELL_ATTR\d+_[A-Z_]+') | ForEach-Object Value | Sort-Object -Unique
$constants=foreach($name in $attributes){if($name -like '*_CU_*'){continue};$match=[regex]::Match($defines,"\b$($name)\s*=\s*(0x[0-9A-Fa-f]+)");if(!$match.Success){throw "Missing native attribute $name"};$index=[regex]::Match($name,'ATTR(\d+)').Groups[1].Value;"constexpr Attribute $name{$index,$($match.Groups[1].Value)};"}
[IO.File]::WriteAllLines("$out/TorchAttributes.inc",$constants)
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
foreach($s in @(@('TorchSpellHit','        case SMART_EVENT_SPELLHIT_TARGET:','        case SMART_EVENT_OOC_LOS:'),@('TorchAction','        case SMART_ACTION_CALL_KILLEDMONSTER:','        case SMART_ACTION_SET_INST_DATA:'))){
 $a=$smart.IndexOf($s[1]);$b=$smart.IndexOf($s[2],$a);if($a -lt 0 -or $b -le $a){throw "Missing production $($s[0])"}
 [IO.File]::WriteAllText("$out/$($s[0]).inc",$smart.Substring($a,$b-$a))
}
& "$PSScriptRoot/Test-StormheimBlightcaller.ps1" *> "$out/kill-upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-blightcaller/*.inc" | Copy-Item -Destination $out
$prefix=Get-Content "$out/BlightFixture.inc" -Raw
$prefix=$prefix.Replace('GetEntry()==91158&&q==38361','GetEntry()==94318&&q==39472')
[IO.File]::WriteAllText("$out/GreywatchKillFixture.inc",$prefix)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$visual=Get-Content "$repo/src/server/scripts/Legion/stormheim_greywatch.cpp" -Raw
$visual=[regex]::Replace($visual,'(?m)^#include.*\r?\n','');$visual=$visual.Substring(0,$visual.IndexOf('void AddSC_stormheim_greywatch()'))
[IO.File]::WriteAllText("$out/GreywatchVisual.inc",$visual)
foreach($test in @('StormheimGreywatchTest','StormheimCurseheartTest','StormheimGreywatchVisualTest')){
 $cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/$test.cpp`" /Fe`"$out/$test.exe`" /Fo`"$out/$test.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/$test.exe`" `"$out/world.tsv`"`r`n"
 [IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
 if($LASTEXITCODE){throw "Greywatch native regression failed: $test"}
}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('spell_stormheim_blight_torch','spell_stormheim_blight_throw','Greywatch: native torch throw and exact cache burn')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc -or (Get-Item "$repo/src/server/scripts/Legion/stormheim_greywatch.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw 'Canonical Release predates Greywatch script implementation'}
 'PASS: canonical Release bindings/unique Greywatch diagnostic and actual scripts final-link input freshness.'
}
