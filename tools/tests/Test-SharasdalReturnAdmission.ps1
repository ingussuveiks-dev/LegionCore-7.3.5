param([switch]$PrepareHost,[string]$DataFile='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$DataFile -and !$PrepareHost){
 & "$PSScriptRoot/Test-SharasdalAcquisitionData.ps1"
 if(!$?){throw 'Fresh tested return condition graph unavailable'}
 $DataFile="$repo/build-extractors/runtime-test/sharasdal-acquisition-data/return-rows.tsv"
}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-return-admission"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};$s.Substring($a,$b-$a)}
$hostSource=Get-Content "$PSScriptRoot/StromkarExecuteLandingTest.cpp" -Raw;$hostSource=$hostSource.Substring(0,$hostSource.IndexOf('struct SpellInfo'))
$hostSource=$hostSource.Replace('../../src/server/game/Movement/SkyholdDeparture.h',"$($repo.Replace('\','/'))/src/server/game/Movement/SkyholdDeparture.h")
$hostSource=$hostSource.Replace('using uint32=uint32_t;','using uint32=uint32_t;using int32=int32_t;using int8=int8_t;')
$hostSource=$hostSource.Replace('CONDITION_AURA=1,','CONDITION_ITEM=2,CONDITION_TYPE_MASK=32,CONDITION_QUEST_OBJECTIVE_DONE=41,CONDITION_AURA=1,')
$hostSource=$hostSource.Replace('    bool IsPlayer()', '    bool isType(uint32 mask){return IsPlayer() && (mask & 16);} virtual uint32 GetEntry()const{return 0;} virtual uint32 GetGUID()const{return 1;} uint32 GetOwnerGUID(){return 0;} struct Vehicle{uint32 GetCreatureEntry(){return 0;}};Vehicle* GetVehicleKit(){return nullptr;} bool HandleSpellClick(Unit*,int8); bool IsPlayer()')
$hostSource=$hostSource.Replace('identity==206448&&effect==0','identity==209684&&effect==0')
$hostSource=$hostSource.Replace('uint32 map=1539,classMask=1','uint32 map=1600,classMask=64').Replace('alive=true,aura=true','alive=true,aura=false')
$hostSource=$hostSource.Replace('identity=1037,step=9','identity=1066,step=6')
$hostSource=$hostSource.Replace('struct Creature : Unit {','struct Creature : Unit {uint32 GetEntry()const override{return 105819;}uint32 GetGUID()const override{return 2;} bool clickable=true;bool HasFlag(uint32,uint32)const{return clickable;}bool isCanTrainingOf(Player const*,bool)const{return false;}')
$hostSource=$hostSource.Replace('    uint32 quest=41105','    uint32 objective=1;bool owned=true,bank=false;bool HasItemCount(uint32 id,uint32 count,bool withBank)const{assert(id==128911&&count==1);return owned||(withBank&&bank);} int32 GetQuestObjectiveData(uint32,uint32)const;int32 GetQuestObjectiveData(struct Quest const*,int8 index)const{assert(index==1);return objective;} bool canSeeSpellClickOn(Creature const*)const; uint32 quest=40341')
$hostSource=$hostSource.Replace('explicit ConditionSourceInfo(Unit* actor):mConditionTargets{actor,nullptr}', 'explicit ConditionSourceInfo(Unit* actor,Unit* other=nullptr):mConditionTargets{actor,other}')
$hostSource=$hostSource.Replace('struct ConditionMgr {','using CreatureSpellConditionContainer=std::map<uint32,ConditionTypeContainer>;struct ConditionMgr {CreatureSpellConditionContainer SpellClickEventConditionStore;ConditionList GetConditionsForSpellClickEvent(uint32,uint32);')
$hostSource=$hostSource.Replace('#include "NotGroupedConditions.inc"','#include "NotGroupedConditions.inc"' + "`n#include `"SpellClickConditions.inc`"")
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw;$cases=''
foreach($pair in @(@('AURA','ITEM'),@('ITEM','ITEM_EQUIPPED'),@('CLASS','RACE'),@('MAPID','AREAID'),@('ALIVE','HP_VAL'),@('SCENARION_STEP','REALM_ACHIEVEMENT'),@('QUESTTAKEN','QUEST_COMPLETE'),@('QUEST_COMPLETE','QUEST_NONE'),@('TYPE_MASK','RELATION_TO'),@('QUEST_OBJECTIVE_DONE','INSTANCE_INFO'))){$cases+=Section $source "        case CONDITION_$($pair[0]):" "        case CONDITION_$($pair[1]):"}
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Block $source 'bool ConditionMgr::IsObjectMeetToConditionList('))
[IO.File]::WriteAllText("$out/NotGroupedConditions.inc",(Block $source 'ConditionList ConditionMgr::GetConditionsForNotGroupedEntry('))
[IO.File]::WriteAllText("$out/SpellClickConditions.inc",(Block $source 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent('))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/CanSee.inc",(Block $player 'bool Player::canSeeSpellClickOn('))
[IO.File]::WriteAllText("$out/QuestObjective.inc",(Block $player 'int32 Player::GetQuestObjectiveData(uint32 QuestID,'))
$refresh=Block $player 'void Player::UpdateForQuestWorldObjects('
[IO.File]::WriteAllText("$out/Refresh.inc",(Section $refresh '            if (obj->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK))' '            if (!buildUpdateBlock && obj->HasFlag(UNIT_FIELD_NPC_FLAGS2,'))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$click=Block $unit 'bool Unit::HandleSpellClick('
[IO.File]::WriteAllText("$out/Click.inc",$click.Substring(0,$click.IndexOf('        SpellInfo const* spellEntry'))+"        assert(caster==clicker&&target==this&&origCasterGUID==clicker->GetGUID());++caster->casts;res=true;return res;`n    }`n    return res;`n}`n")
$flags=Section $unit '            if (index == UNIT_FIELD_NPC_FLAGS)' '            else if (index == UNIT_FIELD_NPC_FLAGS2)'
[IO.File]::WriteAllText("$out/Flags.inc",$flags)
if(!(Get-Content "$repo/src/server/game/Entities/Object/Updates/UpdateFieldFlags.cpp" -Raw).Contains('UF_FLAG_PUBLIC | UF_FLAG_DYNAMIC,                       // UNIT_FIELD_NPC_FLAGS')){throw 'Native NPC flags are no longer dynamic'}
if(!(Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw).Contains('_fieldNotifyFlags           = UF_FLAG_DYNAMIC;')){throw 'Native dynamic field notification default changed'}
if($PrepareHost){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalReturnAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$DataFile`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual native return admission/visibility/refresh failed'}
