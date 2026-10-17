param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-horde-shields"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$PSScriptRoot/Test-StormheimShields.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/alliance-regression.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-shields/*.inc" | Copy-Item -Destination $out
& "$PSScriptRoot/Test-StormheimShieldsData.ps1" -BothFactions
Copy-Item "$repo/build-extractors/runtime-test/stormheim-shields/world.tsv" "$out/world.tsv"
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('uint64 race=1;','uint64 race=2;').Replace('team=ALLIANCE','team=HORDE')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$old=(& git -C $repo show 0d9b4fc5:src/server/scripts/Legion/stormheim_skold_ashil.cpp) -join "`n"
$a=$old.IndexOf('class go_stormheim_engraved_shield');$b=$old.IndexOf('struct npc_stormheim_ascendant_statue')
[IO.File]::WriteAllText("$out/OldShieldAI.inc",$old.Substring($a,$b-$a).Replace('go_stormheim_engraved_shield','old_shield_ai'))
$s=Get-Content "$PSScriptRoot/StormheimShieldsTest.cpp" -Raw
$s=$s.Replace('for(auto id:{39060u,39061u,39472u})','for(auto id:{39385u})')
$s=$s.Replace('39063','38878').Replace('39062','39154').Replace('39092','39155').Replace('39472','39385').Replace('lorna.entry=95921','lorna.entry=95611').Replace('p.race=1','p.race=2').Replace('travelPlayer.race=1','travelPlayer.race=2').Replace('horde.race=2','horde.race=1')
$s=$s.Replace('#include "ShieldsProduction.inc"',"#include `"ShieldsProduction.inc`"`n#include `"OldShieldAI.inc`"")
$needle=' std::array<uint32,6> order'
$extra=@'
 {Player p;p.Accept(38878);old_shield_ai legacy;std::vector<std::unique_ptr<GameObjectAI>> oldAis;for(auto& row:shields){row.second.scripted=legacy.GetAI(&row.second);oldAis.emplace_back(row.second.scripted);row.second.flags=0;row.second.Use(&p);assert(p.GetQuestObjectiveData(38878,row.first)==1);}assert(!p.GetQuestObjectiveData(38878,109346)&&p.GetQuestStatus(38878)==QUEST_STATUS_INCOMPLETE);StormheimSkoldAshil::RepairReadings(&p);assert(p.GetQuestObjectiveData(38878,109346)==6&&p.GetQuestStatus(38878)==QUEST_STATUS_COMPLETE);for(auto& row:shields)row.second.scripted=ais[std::distance(shields.begin(),shields.find(row.first))].get();}
 for(unsigned read=0;read<=6;++read){Player p;p.Accept(38878);for(unsigned i=0;i<read;++i)p.data[{38878,StormheimSkoldAshil::Shields[i].entry}]=1;StormheimSkoldAshil::RepairReadings(&p);assert(p.GetQuestObjectiveData(38878,109346)==int(read));auto packets=p.packets;StormheimSkoldAshil::RepairReadings(&p);assert(p.packets==packets);}
 {Player p;p.Accept(38878);p.Accept(39063);p.data[{38878,109346}]=6;StormheimSkoldAshil::RepairReadings(&p);assert(p.GetQuestObjectiveData(38878,109346)==6);auto& go=shields[242673];go.flags=0;go.Use(&p);assert(!p.GetQuestObjectiveData(39063,109346));}
 for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){Player p;p.Accept(38878);p.quests[38878].Status=status;p.data[{38878,109346}]=1;p.data[{38878,242673}]=p.data[{38878,251557}]=1;StormheimSkoldAshil::RepairReadings(&p);assert(p.GetQuestObjectiveData(38878,109346)==1);}
'@
$s=$s.Replace($needle,$extra+"`n"+$needle)
[IO.File]::WriteAllText("$out/HordeShieldsTest.cpp",$s)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$out/HordeShieldsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Horde shield regression failed'}
'PASS: old shared AI leaves six Horde GO goals with0/6 aggregate; selected-faction personal credit and nondecreasing0..6 saved-goal repair, repeat and stale opposite-faction aggregate isolation.'
if($VerifyReleaseBinary){
 $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$text.Contains('Skold-Ashil: faction shield readings and saved-goal recovery')){throw 'Canonical Release lacks Horde shield recovery'}
 'PASS: canonical Release faction shield marker and actual final-link freshness.'
}
