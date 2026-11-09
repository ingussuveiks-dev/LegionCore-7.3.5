"""Extract selected production lifetimes; baseline pinned before this batch."""
from pathlib import Path
import subprocess,json,sys,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
plan=json.loads((r/'tools/tests/FourteenthTenRepairs.json').read_text(encoding='utf8'))
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show','f382f0c19:'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 s=source(e,old);k=e['key'];marker=e['edits'][0]['old'if old else'new']
 assert s.count(marker)==1
 if k in ['Molten','Brawler','Cocoon']:
  code=block(s,marker.split('\n')[0])+');'
  return 'void Start(Player*player,Creature*summon,Creature*spider,Creature*kokon,Unit*caster){'+code+'}'
 if k=='Telarn':return 'void Start(){if(me->IsAlive() && instance){'+marker+'}}'
 if k=='Dramock':
  c=block(s,'struct npc_dramock:');c=c[c.index('{')+1:-1]
  return c.replace('explicit npc_dramock(Creature* creature) : ScriptedAI(creature) {}','')
 if not old:return marker
 return block(s,'void OnPlayerEnter(')
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);body=selected(e,sys.argv[3]=='baseline').replace(' override','')
 alias='using instance_the_nightnold_InstanceMapScript=Frame;'if e['key']=='NightholdEntry'else''
 (out/'Selected.inc').write_text('struct Frame:Base{using Base::Base;'+alias+body+'};',encoding='utf8')
else:
 parts=[]
 for e in plan:
  k=e['key'];s=selected(e,False).replace(' override','')
  if k=='Molten':s=s.replace('me->CastSpell(summon, SPELL_MOLTEN_TOUCH_CHANNEL, true);','Record("Molten");')
  elif k=='Brawler':s=s.replace('summon->GetMotionMaster()->Clear();','').replace('summon->GetMotionMaster()->MovePath(team == TEAM_ALLIANCE ? a_WP[urand(0, 5)] : h_WP[urand(0, 5)], false);','Record(team == TEAM_ALLIANCE ? "Alliance" : "Horde");')
  elif k=='Cocoon':
   s=s.replace('spider->CastSpell(kokon, 116894);','Record("CocoonRelease");').replace('kokon->DespawnOrUnsummon(3000);','Record("CocoonCleanup");').replace('spider->GetMotionMaster()->MoveJump(spider->GetPositionX(), spider->GetPositionY(), caster->GetPositionZ(), 50, 50);','Record("CocoonJump");')
  elif k=='Telarn':s=s.replace('Init();','Record("Telarn");')
  elif k in ['StormEntry','AzsunaEntry','HMEntry','AssaultEntry','NightholdEntry']:
   # Actual entry-attempt lifetime/map guards, payload endpoint captured.
   a=s.index('auto attempt =');b=s.index('if (player->GetQuestStatus',a)if k in ['StormEntry','AzsunaEntry','HMEntry']else s.index('player->CastSpell',a)if k=='AssaultEntry'else s.index('if (GetBossState',a)
   setup=s[a:b];callback='''player->AddDelayedEvent(5000, [player, mapId, instanceId, weakAttempt](){
    if (weakAttempt.expired() || !player->IsInWorld() || player->GetMapId() != mapId || player->GetInstanceId() != instanceId)
        return;
    Record("'''+k+'''");});'''
   # Assert these guards/captures are present verbatim in the selected source.
   assert 'if (weakAttempt.expired() || !player->IsInWorld() || player->GetMapId() != mapId || player->GetInstanceId() != instanceId)'in s
   s='std::map<ObjectGuid,std::shared_ptr<uint8>> entryAttempts;'+block(s,'void OnPlayerLeave(')+'void OnPlayerEnter(Player*player){if(!player)return;'+setup+callback+'}'
  else:
   # Actual GUID snapshot consumption/current quest guard, real native credit replaced.
   a=s.index('std::list<ObjectGuid> eligiblePlayers;');b=s.index('break;',a);c=s[a:b].rsplit('}',1)[0]
   c=c.replace('player->KilledMonsterCredit(me->GetEntry());','Record("Dramock");')
   s='std::list<ObjectGuid>playerGUIDs;void Finish(){'+c+'}'
  parts.append('struct N'+k+':NativeBase{using NativeBase::NativeBase;'+s+'};')
 (out/'FourteenthNativeScripts.inc').write_text('\n'.join(parts),encoding='utf8')
