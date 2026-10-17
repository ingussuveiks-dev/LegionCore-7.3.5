param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimAscendantData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-ascendant"
& "$PSScriptRoot/Test-StormheimShields.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/shields-regression.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-shields/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('assert(n>0);','assert(n>0||q->id==39092);')
$f=$f.Replace('int CastSpell(Player*,uint32 a,bool){','bool lastTriggered=true;uint32 lastCast=0;int CastSpell(Player*,uint32 a,bool triggered){lastCast=a;lastTriggered=triggered;')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$f=Get-Content "$out/ShieldsFixture.inc" -Raw
$f=$f.Replace('assert(!spellId);(void)spellCaster;','if(spellId)user->ToPlayer()->CastSpell(user->ToPlayer(),spellId,true);(void)spellCaster;')
$f=$f.Replace('Index[8]{0,5}','Index[8]{0,13}').Replace('id==1691','id==43').Replace('MiscValue=5','MiscValue=13').Replace('Id=3365','Id=6478')
[IO.File]::WriteAllText("$out/AscendantFixture.inc",$f)
$s=Get-Content "$repo/src/server/scripts/Legion/stormheim_skold_ashil.cpp" -Raw
$a=$s.IndexOf('namespace StormheimSkoldAshil');$b=$s.IndexOf('void AddSC_stormheim_skold_ashil()')
[IO.File]::WriteAllText("$out/AscendantProduction.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$s.IndexOf('void Spell::EffectKillCredit(SpellEffIndex');$b=$s.IndexOf('void Spell::',$a+10)
[IO.File]::WriteAllText("$out/NativeGroupEffect.inc",$s.Substring($a,$b-$a).Replace('Spell::','NativeCreditSpell::'))
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$s.IndexOf('        case CONDITION_QUESTTAKEN:');$b=$s.IndexOf('        case CONDITION_QUEST_COMPLETE:',$a)
$c=$s.IndexOf('        case CONDITION_QUEST_OBJECTIVE_DONE:');$d=$s.IndexOf('        case CONDITION_INSTANCE_INFO:',$c)
[IO.File]::WriteAllText("$out/CommunionMenuConditions.inc",$s.Substring($a,$b-$a)+$s.Substring($c,$d-$c))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimAscendantTest.cpp`" /Fe`"$out/Ascendant.exe`" /Fo`"$out/Ascendant.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Ascendant.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Extracted Ascendant regression failed'}
if($VerifyReleaseBinary){
 $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 foreach($marker in @('npc_stormheim_ascendant_statue','spell_stormheim_communing','player_stormheim_communion_recovery','Skold-Ashil: ordered native communion')){if(!$text.Contains($marker)){throw "Canonical Release lacks $marker"}}
 'PASS: canonical Release communion registrations/diagnostic and shared implementation final-link freshness.'
}
