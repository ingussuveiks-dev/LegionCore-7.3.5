$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimRunesBindData.ps1"
& "$PSScriptRoot/Test-StormheimUnworthyTask.ps1" *> "$repo/.codex/runes-bind-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-runes-bind"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-unworthy-task/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/UnworthyFixture.inc" -Raw
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {bool isGameMaster(){return false;}bool HasQuestForGO(int32);bool HasQuestForItem(uint32){return false;}')
$fixture=$fixture.Replace('struct GameObjectTemplate {','struct GameObjectTemplate {uint32 QuestItems[6]{};struct{int32 questID=0;}generic,spellFocus;struct{bool usegrouplootrules=false;}chest;')
$fixture=$fixture.Replace('GameObjectTemplate const* GetGOInfo(){','bool ActivateToQuest(Player*)const;uint32 GetGoType()const{return info.type;}GameObjectTemplate const* GetGOInfo()const{')
$fixture=$fixture.Replace('uint32 GetEntry(){','uint32 GetEntry()const{')
$fixture=$fixture.Replace('struct QuestStore {','struct QuestStore {bool IsGameObjectForQuests(uint32 id){return id==241849||id==251412||id==251413;}')
$fixture=$fixture.Replace('struct BG {','struct BG {virtual ~BG()=default;int GetTypeID(bool){return 0;}')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$fixture)
$source=Get-Content "$PSScriptRoot/StormheimUnworthyTaskTest.cpp" -Raw
[IO.File]::WriteAllText("$out/RunesBindFixture.inc",$source.Substring(0,$source.IndexOf('int main(')))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$player.IndexOf('bool Player::HasQuestForGO(');$b=$player.IndexOf('void Player::UpdateForQuestWorldObjects(',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native GO objective visibility check'}
[IO.File]::WriteAllText("$out/HasQuestForGO.inc",$player.Substring($a,$b-$a).Replace('HasQuestForGO(int32 GOId) const','HasQuestForGO(int32 GOId)'))
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$go.IndexOf('bool GameObject::ActivateToQuest(');$b=$go.IndexOf('void GameObject::TriggeringLinkedGameObject(',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native GO activation check'}
[IO.File]::WriteAllText("$out/ActivateToQuest.inc",$go.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimRunesBindTest.cpp`" /Fe`"$out/RunesBind.exe`" /Fo`"$out/RunesBind.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/RunesBind.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Runes Bind native interaction regression failed'}
