"""Generate isolated fixtures using retained, actual native loader/packet/loot code."""
from pathlib import Path
import sys,re
from layered_ten_ten import load,filename,ROOT,pred
from prepare_layered_tests import setup,batches
kind=sys.argv[1];out=Path(sys.argv[2]);sql_dir=Path(sys.argv[3]);p=load(kind);rows=p['repairs']
def extract(s,start):
 a=s.index(start);i=s.index('{',a);depth=1;j=i+1
 while depth:depth+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[a:j]
def chunks(s):return 'std::string('+')+std::string('.join('R"LYRD('+s[i:i+16000]+')LYRD"' for i in range(0,len(s),16000))+')'
def array(name,s):
 s='\n'.join(l for l in s.splitlines() if not l.startswith('--'));return 'static std::string const '+name+'[]={'+','.join(chunks(x) for x in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",s) if x.strip())+'};'
migrations='\n'.join((sql_dir/filename(kind,n)).read_text(encoding='utf-8') for n in batches(p))
if kind=='names':
 lines=['struct NameRow{uint32 entry;char const* before,*after;};static NameRow const NameRows[]={'+','.join('{'+str(r['entry'])+',"'+r['before_hex']+'","'+r['after_hex']+'"}' for r in rows)+'};','static char const* NameIds="'+','.join(str(r['entry']) for r in rows)+'";',array('NameMigrations',migrations)]
 (out/'GameObjectNameRows.inc').write_text('\n'.join(lines),encoding='utf-8')
 source=(ROOT/'tools/tests/GameObjectNameNative.cpp').read_text(encoding='utf-8').replace('WorldDatabase.DirectExecute(sql);','WorldDatabase.DirectExecute(sql.c_str());').replace('PASS100','PASS37')
else:
 base=(ROOT/'tools/tests/RockawayChannelNative.cpp').read_text(encoding='utf-8');lines=[base[:base.index('// The scheduler lookup')], '\n#include "WorldSession.h"\n#include "LootMgr.h"\n#include "CellImpl.h"\n#include "GameObject.h"\n#include "GameObjectData.h"\n#include "Opcodes.h"\n']
 offensive=(ROOT/'tools/tests/RockawayOffensiveNative.cpp').read_text(encoding='utf-8');player=extract(offensive,'struct NativePlayer : Player');a=player.index('        // Controlled accepted-task state;');b=player.index('\n    }',a);lines.append(player[:a]+player[b:]+';')
 lines += [array('SetupSql',setup(p,kind,'').split('SET SESSION TRANSACTION READ ONLY;')[0]),array('Migrations',migrations),array('ResetSql','\n'.join(f"UPDATE gameobject_template g SET Data1=0 WHERE {pred(r,kind)};" for r in rows)),'static uint32 const Entries[]={'+','.join(str(r['entry']) for r in rows)+'};','static uint32 const LootRowCount='+str(sum(r['loot_rows'] for r in rows))+';']
 lines.append('#include <DbgHelp.h>\n#pragma comment(lib, "Dbghelp.lib")\n'+extract(offensive,'static LONG WINAPI CrashTrace'))
 qsource=(ROOT/'src/server/game/Globals/QuestData.cpp').read_text(encoding='utf-8');a=qsource.index('QueryResult result = WorldDatabase.Query("SELECT "',qsource.index('void QuestDataStoreMgr::LoadQuests()'));b=qsource.index(');',a);query=''.join(re.findall(r'"([^"]*)"',qsource[a:b]));assert query.endswith(' FROM quest_template')
 qids=sorted({r['quest_gate']['quest'] for r in rows if r['quest_gate']});ids=','.join(map(str,qids));lines.append('static std::string const QuestQuery='+chunks(query+' WHERE ID IN('+ids+')')+';')
 lines.append('static char const* QuestIds="'+ids+'";static uint32 const QuestBound='+str(max(qids)+1)+';')
 lines.append('struct GateRow{uint32 entry,item,quest,objective,amount;};static GateRow const GateRows[]={'+','.join('{'+','.join(str(v) for v in (r['entry'],*[r['quest_gate'][k] for k in ('item','quest','objective','amount')]))+'}' for r in rows if r['quest_gate'])+'};')
 (out/'NativeFollowingGoLoot.inc').write_text('\n'.join(lines),encoding='utf-8')
 source=(ROOT/'tools/tests/FollowingGoLootNative.cpp').read_text(encoding='utf-8').replace('PASS100','PASS63')
 marker='static void Init(int argc,char** argv)'
 helpers='''
struct QuestVectorTag{using Type=QuestVector QuestDataStoreMgr::*;friend Type Access(QuestVectorTag);};
struct QuestMaxTag{using Type=uint32 QuestDataStoreMgr::*;friend Type Access(QuestMaxTag);};
template struct Accessor<QuestVectorTag,&QuestDataStoreMgr::_questVTemplates>;
template struct Accessor<QuestMaxTag,&QuestDataStoreMgr::_maxQuestId>;
static void LoadQuestGates(){
 auto& quests=sQuestDataStore->*Access(QuestVectorTag{});quests.resize(QuestBound);sQuestDataStore->*Access(QuestMaxTag{})=QuestBound;
 auto rows=WorldDatabase.Query(QuestQuery.c_str());Require(bool(rows),"Actual quest rows missing");do{auto q=new Quest(rows->Fetch());Require(q->GetQuestId()<QuestBound,"Quest bound differs");quests[q->GetQuestId()]=q;const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[q->GetQuestId()]=q;}while(rows->NextRow());
 auto obj=WorldDatabase.Query((std::string("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(")+QuestIds+") ORDER BY StorageIndex ASC").c_str());Require(bool(obj),"Actual objectives missing");do{auto f=obj->Fetch();auto q=quests[f[1].GetUInt32()];Require(q,"Objective quest missing");q->LoadQuestObjective(f);if(f[2].GetUInt8()==QUEST_OBJECTIVE_ITEM)q->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);}while(obj->NextRow());
 for(auto const& gate:GateRows){auto q=quests[gate.quest];bool found=false;for(auto const& o:q->GetObjectives())if(o.ID==gate.objective&&o.Type==QUEST_OBJECTIVE_ITEM&&uint32(o.ObjectID)==gate.item&&uint32(o.Amount)==gate.amount)found=true;Require(found,"Captured native item objective changed");}
}
static GateRow const* Gate(uint32 entry){for(auto const& r:GateRows)if(r.entry==entry)return &r;return nullptr;}
static void CheckGate(Player& owner,GateRow const& gate){
 // Controlled accepted log state only. No quest acceptance, save, progress or reward.
 owner.SetQuestSlot(0,gate.quest,0);Loot loot;auto item=sObjectMgr->GetItemTemplate(gate.item);Require(item,"Gate item absent");Require(!(item->FlagsCu&ITEM_FLAGS_CU_IGNORE_QUEST_STATUS),"Quest source bypasses normal status gate; inspect separately");
 for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){owner.SetQuestStatus(gate.quest,status);bool active=status==QUEST_STATUS_INCOMPLETE;Require(owner.HasQuestForItem(gate.item)==active,"Actual quest-item eligibility changed");bool faction=(!((item->GetFlags2()&ITEM_FLAG2_FACTION_HORDE)&&owner.GetTeam()!=HORDE))&&(!((item->GetFlags2()&ITEM_FLAG2_FACTION_ALLIANCE)&&owner.GetTeam()!=ALLIANCE));Require(loot.AllowedForPlayer(&owner,gate.item,0,LOOT_ITEM_TYPE_ITEM,true,nullptr)==(active&&faction),"Actual native loot quest/faction filter changed");}
 owner.SetQuestStatus(gate.quest,QUEST_STATUS_NONE);owner.SetQuestSlot(0,0,0);
}
'''
 source=source.replace(marker,helpers+'\n'+marker).replace('opcodeTable.Initialize();','LoadQuestGates();opcodeTable.Initialize();')
 source=source.replace('auto info=sObjectMgr->GetGameObjectTemplate(entry);','auto info=sObjectMgr->GetGameObjectTemplate(entry);if(auto gate=Gate(entry))CheckGate(owner,*gate);')
 source=source.replace('     {Chest source(map,entry,guid++,pos);','     auto gate=Gate(entry);if(gate){owner.SetQuestSlot(0,gate->quest,0);owner.SetQuestStatus(gate->quest,QUEST_STATUS_INCOMPLETE);}\n     {Chest source(map,entry,guid++,pos);')
 source=source.replace('Generation stored an inventory item");}', 'Generation stored an inventory item");}\n     if(gate){owner.SetQuestStatus(gate->quest,QUEST_STATUS_NONE);owner.SetQuestSlot(0,0,0);}')
 source=source.replace('no client/lock/placement/drop/inventory claim','29 actual quest/objective inactive/incomplete/complete and loot filters; no client/lock/placement/drop/inventory claim')
(out/'LayeredNative.cpp').write_text(source,encoding='utf-8')
