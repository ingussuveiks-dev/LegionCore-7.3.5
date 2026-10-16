$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimBlightcallerData.ps1"
& "$PSScriptRoot/Test-StormheimBreakSpine.ps1" *> "$repo/.codex/blightcaller-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-blightcaller";$upstream="$repo/build-extractors/runtime-test/stormheim-break-spine"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/SpineFixture.inc" -Raw
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$fixture=$fixture.Replace('std::set<ObjectGuid> rewarded;','std::set<ObjectGuid> rewarded;bool IsPlayerRewarded(ObjectGuid guid)const{return rewarded.count(guid);}')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {uint32 getLevel(){return 100;}void KilledPlayerCredit(){}')
[IO.File]::WriteAllText("$out/SpineFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/StormheimBreakSpineTest.cpp" -Raw
$prefix=$prefix.Substring(0,$prefix.IndexOf('int main(')).Replace('GetEntry()==97986&&q==40004','GetEntry()==91158&&q==38361')
$prefix=$prefix.Replace('void _RewardPlayer(Player* p,bool){_RewardKillCredit(p);}','bool _isBattleGround=false,_isPvP=false;uint32 _xp=0;float _groupRate=1,_sumLevel=100;void _RewardHonor(Player*){}void _RewardXP(Player*,float){}void _RewardReputation(Player*,float){}void _RewardPlayer(Player*,bool);')
[IO.File]::WriteAllText("$out/BlightFixture.inc",$prefix)
$reward=Get-Content "$repo/src/server/game/Entities/Player/KillRewarder.cpp" -Raw
$a=$reward.IndexOf('void KillRewarder::_RewardPlayer(');$b=$reward.IndexOf('void KillRewarder::_RewardGroup(',$a)
[IO.File]::WriteAllText("$out/NativeRewardPlayer.inc",$reward.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimBlightcallerTest.cpp`" /Fe`"$out/Blightcaller.exe`" /Fo`"$out/Blightcaller.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Blightcaller.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Blightcaller native regression failed'}
