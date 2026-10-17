$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimDilemmaData.ps1"
$out="$repo/build-extractors/runtime-test/stormheim-dilemma"
& "$PSScriptRoot/Test-StormheimBlightcaller.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-blightcaller/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/SpineFixture.inc" -Raw
$f=$f.Replace('struct QuestStatusData {','struct QuestStatusData {std::map<int8,int32> ObjectiveData;')
$f=$f.Replace('int32 GetQuestObjectiveData(uint32 q,uint32 e){return data[{q,e}];}','int32 GetQuestObjectiveData(uint32,uint32)const;')
$f=$f.Replace('int32 GetQuestObjectiveData(Quest const* q,int8 index){for(auto const& o:q->objectives)if(o.StorageIndex==index)return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}','int32 GetQuestObjectiveData(Quest const*,int8)const;')
$f=$f.Replace('void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,int32 n){data[{q->id,o->ObjectID}]=n;}','void SetQuestObjectiveData(Quest const*,QuestObjective const*,int32,bool=false);')
$f=$f.Replace('struct QuestObjective {','struct QuestObjective {bool IsStoringFlag()const{return false;}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {std::map<uint32,uint32> m_QuestStatusSave;std::map<int8,int32> slotCounters;uint16 FindQuestSlot(uint32){return 0;}void SetQuestSlotCounter(uint16,int8 index,int32 n){slotCounters[index]=n;}void SetQuestSlotState(uint16,int){}void RemoveQuestSlotState(uint16,int){}void SetSpecialCriteriaComplete(uint16,int8){}void SendDirectMessage(int){}')
$f=$f.Replace('struct Achievement {','struct Achievement {bool IsCompletedCriteriaTree(void const*){return false;}')
[IO.File]::WriteAllText("$out/SpineFixture.inc",$f)
$f=(Get-Content "$out/BlightFixture.inc" -Raw).Replace('GetEntry()==91158&&q==38361','GetEntry()==93603&&q==39153')
[IO.File]::WriteAllText("$out/DilemmaFixture.inc",$f)
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$native.IndexOf('int32 Player::GetQuestObjectiveData(uint32');$b=$native.IndexOf('void Player::SetQuestObjectiveData(',$a)
[IO.File]::WriteAllText("$out/NativeDilemmaGetters.inc",$native.Substring($a,$b-$a))
$a=$b;$b=$native.IndexOf('bool Player::HasCompletedQuest(',$a)
[IO.File]::WriteAllText("$out/NativeDilemmaSetter.inc",$native.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimDilemmaTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dilemma native regression failed'}
