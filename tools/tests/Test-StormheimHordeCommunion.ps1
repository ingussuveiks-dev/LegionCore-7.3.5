param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-horde-communion"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$PSScriptRoot/Test-StormheimAscendant.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/alliance-regression.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-ascendant/*.inc" | Copy-Item -Destination $out
& "$PSScriptRoot/Test-StormheimAscendantData.ps1" -BothFactions
Copy-Item "$repo/build-extractors/runtime-test/stormheim-ascendant/world.tsv" "$out/world.tsv"
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('uint64 race=1;','uint64 race=2;').Replace('team=ALLIANCE','team=HORDE').Replace('n>0||q->id==39092','n>0||q->id==39092||q->id==39155')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$old=(& git -C $repo show 05c5f300:src/server/scripts/Legion/stormheim_skold_ashil.cpp) -join "`n"
$a=$old.IndexOf('namespace StormheimSkoldAshil');$b=$old.IndexOf('class go_stormheim_engraved_shield')
[IO.File]::WriteAllText("$out/OldCommunion.inc",$old.Substring($a,$b-$a).Replace('StormheimSkoldAshil','OldSkoldAshil'))
$s=Get-Content "$PSScriptRoot/StormheimAscendantTest.cpp" -Raw
$s=$s.Replace('39092','39155').Replace('39122','38882').Replace('39063','38878').Replace('genn.entry=95804','genn.entry=94228')
$s=$s.Replace('#include "AscendantProduction.inc"',"#include `"AscendantProduction.inc`"`n#include `"OldCommunion.inc`"")
$s=$s.Replace('assert(!negative&&!c.ElseGroup);','assert(!negative&&c.ElseGroup<=1);').Replace('gates.size()==5','gates.size()==10')
$s=$s.Replace('int main(int argc,char**argv){','std::list<Condition> oldMenu;ConditionList oldGates;int main(int argc,char**argv){')
$needle='  if(kind=="condition")'
$oldParse='  if(kind=="old-condition"){oldMenu.emplace_back();auto& c=oldMenu.back();uint32 negative;in>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>negative>>c.ElseGroup;oldGates.push_back(&c);}'
$s=$s.Replace($needle,$oldParse+"`n"+$needle)
$needle='npc_stormheim_ascendant_statue ai(&statue);'
$extra=@'
 {Player p;p.Accept(39155);Four(p);assert(oldGates.size()==5&&!conditionMgr.IsObjectMeetToConditions(&p,oldGates)&&!OldSkoldAshil::Communion(&p,&statue));assert(conditionMgr.IsObjectMeetToConditions(&p,gates)&&StormheimSkoldAshil::Communion(&p,&statue));}
 {Player p;p.Accept(39155);p.Accept(39092);p.data[{39155,242994}]=p.data[{39155,242995}]=1;p.data[{39092,242996}]=p.data[{39092,242998}]=1;assert(!conditionMgr.IsObjectMeetToConditions(&p,gates));spell_stormheim_communing cast;cast.caster=&p;cast.hit=&p;assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);p.data[{39155,242996}]=p.data[{39155,242998}]=1;assert(cast.Check()==SPELL_CAST_OK);p.team=ALLIANCE;Finish(cast);assert(!p.GetQuestObjectiveData(39155,94477)&&!p.GetQuestObjectiveData(39092,94477));}
 {Player p;p.Accept(39155);p.Accept(39092);Four(p);spell_stormheim_communing cast;cast.caster=&p;cast.hit=&p;assert(cast.Check()==SPELL_CAST_OK);Finish(cast);assert(p.GetQuestObjectiveData(39155,94477)==1&&!p.GetQuestObjectiveData(39092,94477));}
'@
$s=$s.Replace($needle,$needle+"`n"+$extra)
[IO.File]::WriteAllText("$out/HordeCommunionTest.cpp",$s)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$out/HordeCommunionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Shared statue Horde regression failed'}
'PASS: old Alliance-only menu/helper reject Horde; separate OR groups now admit each faction, reject mixed partial goals/faction changes and credit only the selected quest, including stale opposite-faction records.'
if($VerifyReleaseBinary){
 $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$text.Contains('Skold-Ashil: faction-specific communion quest lifetime')){throw 'Canonical Release lacks faction-specific statue implementation'}
 'PASS: canonical Release shared-statue marker and actual final-link freshness.'
}
