$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimTideskornData.ps1"
$out="$repo/build-extractors/runtime-test/stormheim-tideskorn"
& "$PSScriptRoot/Test-StormheimDilemma.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-dilemma/*.inc" | Copy-Item -Destination $out
function Replace-Tideskorn([string]$text,[string]$old,[string]$new){if(!$text.Contains($old)){throw "Fixture boundary changed: $old"};$text.Replace($old,$new)}
$f=Get-Content "$out/SpineFixture.inc" -Raw
$f=Replace-Tideskorn $f 'struct QuestObjective {' 'struct QuestObjective {float TaskStep=0;'
$f=Replace-Tideskorn $f 'struct Quest {' 'struct Quest {uint32 Type=0;bool IsEmissary()const{return false;}bool IsWorld()const{return false;}'
$f=Replace-Tideskorn $f 'struct QuestStore {' 'struct QuestStore {std::map<uint32,std::set<Quest const*>> tasks;std::set<Quest const*> const* GetQuestTask(uint32 area){auto i=tasks.find(area);return i==tasks.end()?nullptr:&i->second;}'
$f=Replace-Tideskorn $f 'uint16 FindQuestSlot(uint32){return 0;}' 'std::array<uint32,MAX_QUEST_LOG_SIZE> slots{};uint16 FindQuestSlot(uint32 q){for(uint16 i=0;i<slots.size();++i)if(slots[i]==q)return i;return MAX_QUEST_LOG_SIZE;}void SetQuestSlot(uint16 slot,uint32 q){slots.at(slot)=q;}'
$f=Replace-Tideskorn $f 'uint32 getLevel(){return 100;}' 'uint32 level=100;uint32 getLevel(){return level;}'
$f=Replace-Tideskorn $f 'int GetRank(void const*){return 0;}' 'int GetRank(void const*){return 0;}int GetReputation(uint32){return 0;}'
$f=Replace-Tideskorn $f 'bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return GetQuestObjectiveData(q->id,o.ObjectID)>=int32(o.Amount);}' 'bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&);bool HasSpell(uint32){return false;}void UpdateAreaQuestTasks(uint32,uint32);void UpdateQuestObjectiveData(Quest const*);bool IsQuestRewarded(uint32 q){return m_RewardedQuests.count(q);}bool CanAddQuest(Quest const*,bool){return FindQuestSlot(0)<MAX_QUEST_LOG_SIZE;}void TakeQuestSourceItem(uint32,bool){++sourceRemovals;}uint32 sourceRemovals=0;'
$f=Replace-Tideskorn $f 'void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}' 'void CompleteQuest(uint32);uint32 rewards=0;void RewardQuest(Quest const* q,uint32,Object*,bool announce){assert(announce&&q->Type==3&&GetQuestStatus(q->id)==QUEST_STATUS_COMPLETE);++rewards;m_RewardedQuests.insert(q->id);quests[q->id].Status=QUEST_STATUS_REWARDED;SetQuestSlot(FindQuestSlot(q->id),0);}'
$f=Replace-Tideskorn $f 'uint32 GetQuestSlotQuestId(uint8 index){auto i=quests.begin();while(index--&&i!=quests.end())++i;return i==quests.end()?0:i->first;}' 'uint32 GetQuestSlotQuestId(uint8 index){return slots.at(index);}'
$f=Replace-Tideskorn $f 'void AddQuest(Quest const* q,Object*){Accept(q->id);}' 'void AddQuest(Quest const* q,Object*){quests[q->id].Status=QUEST_STATUS_INCOMPLETE;SetQuestSlot(FindQuestSlot(0),q->id);}'
$f=Replace-Tideskorn $f 'void Accept(uint32 q){quests.erase(q);' 'void Accept(uint32 q){SetQuestSlot(FindQuestSlot(0),q);quests.erase(q);'
$f=Replace-Tideskorn $f 'struct SpellInfo {' 'struct SpellInfo {int32 MiscValue=0;'
$f=Replace-Tideskorn $f 'GameObjectTemplate info;' 'bool IsWithinDistInMap(WorldObject*,float){return distance<=5;}int getLootState(){return state;}void SetLootState(uint32 s){state=s;}GameObjectTemplate info;'
$constants='enum {QUEST_OBJECTIVE_TASK_IN_ZONE=15,QUEST_OBJECTIVE_PET_TRAINER_DEFEAT=11,QUEST_OBJECTIVE_PLAYERKILLS=9,QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE=14,QUEST_OBJECTIVE_HAVE_CURRENCY=16,QUEST_OBJECTIVE_OBTAIN_CURRENCY=17,QUEST_OBJECTIVE_MIN_REPUTATION=6,QUEST_OBJECTIVE_MAX_REPUTATION=7,QUEST_OBJECTIVE_AREATRIGGER=10,QUEST_OBJECTIVE_LEARNSPELL=5,QUEST_FLAGS_TRACKING=0x400,QUEST_TYPE_TASK=3,QUEST_STATE_COMPLETE=1,QUEST_REQUIRED=3,QUEST_DEFAULT_SAVE_TYPE=1};'
[IO.File]::WriteAllText("$out/SpineFixture.inc","#include <array>`n$constants`n"+$f)
$f=Replace-Tideskorn (Get-Content "$out/DilemmaFixture.inc" -Raw) 'struct ScriptMgr{' 'struct ScriptMgr{std::function<bool(Player*,GameObject*)> gossip;bool OnGossipHello(Player* p,GameObject* g){return gossip?gossip(p,g):false;}void OnQuestComplete(Player* p,Quest const*){++p->completions;}'
[IO.File]::WriteAllText("$out/TideskornFixture.inc",$f)
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
function Export-Tideskorn([string]$start,[string]$end,[string]$name){$a=$native.IndexOf($start);$b=$native.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Native boundary missing: $name"};[IO.File]::WriteAllText("$out/$name.inc",$native.Substring($a,$b-$a))}
Export-Tideskorn 'bool Player::HasQuestObjectiveComplete(' 'bool Player::CanCompleteQuest(' 'TideskornObjective'
Export-Tideskorn 'void Player::CompleteQuest(' 'void Player::IncompleteQuest(' 'TideskornComplete'
Export-Tideskorn 'void Player::UpdateQuestObjectiveData(' 'void Player::SendQuestComplete(' 'TideskornRestore'
$a=$native.IndexOf('void Player::UpdateAreaQuestTasks(');$b=$native.IndexOf('    if (!sWorld->getBoolConfig(CONFIG_WORLD_QUEST))',$a)
if($a -lt 0 -or $b -le $a){throw 'Native bonus task boundary missing'}
[IO.File]::WriteAllText("$out/TideskornActivation.inc",$native.Substring($a,$b-$a)+"}`n")
$spell=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
if($spell -notmatch 'EffectKillCredit,\s*//134 SPELL_EFFECT_KILL_CREDIT'){throw 'Native effect134 dispatch changed'}
$a=$spell.IndexOf('void Spell::EffectKillCredit(SpellEffIndex');$b=$spell.IndexOf('void Spell::EffectQuestFail(',$a)
[IO.File]::WriteAllText("$out/TideskornSpell.inc",$spell.Substring($a,$b-$a))
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$go.IndexOf('    if (IsQuestObjectExpired())',$go.IndexOf('void GameObject::Use('));$b=$go.IndexOf('    // If cooldown data present in template',$a)
[IO.File]::WriteAllText("$out/TideskornGoAdmission.inc",$go.Substring($a,$b-$a))
$a=$go.IndexOf('    SetUInt32Value(GAMEOBJECT_FIELD_FLAGS, goinfo->flags);');if($a -lt 0){throw 'Native GO initialization changed'}
[IO.File]::WriteAllText("$out/TideskornGoInit.inc",'    SetUInt32Value(GAMEOBJECT_FIELD_FLAGS, goinfo->flags);')
$zone=Get-Content "$repo/src/server/scripts/Legion/zone_azsuna.cpp" -Raw
$a=$zone.IndexOf('class go_grey_shoals_supplies');$b=$zone.IndexOf('void AddSC_azsuna()',$a)
[IO.File]::WriteAllText("$out/TideskornSharedHook.inc",$zone.Substring($a,$b-$a))
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$r.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimTideskornCatalog','StormheimTideskornTest')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++20','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'});$compilerArgs+='/I"'+$out+'"';$compilerArgs+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$name+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$name+'.obj"'
 if($name -like '*Catalog'){$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $name compilation failed"}
}
$previous=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 $rows=@(& "$out/StormheimTideskornCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/");$code=$LASTEXITCODE;$rows | Set-Content "$out/native.tsv";$rows
 if($code -or $rows -notcontains 'area 7643 Gates of Valor'){throw 'Native Tideskorn catalog failed'}
 & "$out/StormheimTideskornTest.exe" "$out/world.tsv" "$out/native.tsv"
 if($LASTEXITCODE){throw 'Native Tideskorn progression failed'}
}finally{$env:PATH=$previous}
