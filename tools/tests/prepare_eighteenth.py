"""Selected production arithmetic/admission paths and instrumented pinned controls."""
from pathlib import Path
import sys,json,subprocess,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/EighteenthTenRepairs.json').read_text());base='b0f910cb2'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def selected(e,old):
 s=subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig');k=e['key']
 if k=='ToppsRay':
  a=s.index('float switch_x =');b=s.index('bool can_cast',a);return 'void Run(){do{'+s[a:b]+'}while(false);}'
 if k in ['FocusAction','LeiShiAction']:
  marker='if ('+('ACTION_1'if k=='FocusAction'else'ACTION_TERMINATE_HIDE_PHASE')+')'if old else'if (action == '+('ACTION_1'if k=='FocusAction'else'ACTION_TERMINATE_HIDE_PHASE')+')';assert s.count(marker)==1
  return 'void Run(int32 action){'+marker+'++accepted;}'
 if k in ['LoryHealth','FreyaHealth']:return 'void Run(Creature*creature){'+e['edits'][0]['old'if old else'new']+'}'
 if k=='MeljarakHeal':
  a=e['edits'][1]['old'if old else'new'];param='uint32'if old else'uint64';return 'void SendHealSoldiers('+param+' amount){other->SetHealth(other->GetHealth()+amount);}void Run(){'+a+'SendHealSoldiers(modhealth);}'
 if k=='GarroshAbsorb':
  a=e['edits'][0]['old'if old else'new'];b=e['edits'][1]['old'if old else'new']
  if old:a=a.replace('int32(GetTarget()->CountPctFromMaxHealth(80))','CounterfactualSigned32(GetTarget()->CountPctFromMaxHealth(80))')
  return 'void Apply(){'+a+'aurEff->SetAmount(newamount);}void Run(){'+b+'}'
 if k=='NightmotherLanding':
  a=s.index('struct npc_en_nightmother');s=s[a:];c=block(s,'case EVENT_2:');return 'void Run(){do{'+c[c.index('{')+1:-1]+'}while(false);}'
 if k=='GlowcapStack':
  a=s.index('if (aura->GetStackAmount() > 100)');b=s.index('if (aura->GetStackAmount() >= 30',a);return 'void Run(){'+s[a:b]+'}'
 if k=='MasterRoster':
  if old:return 'void Reset(){timer_check=500;}void Join(uint64 id){activeAddGuids.insert(id);++count_adds;}void Die(uint64 id){activeAddGuids.erase(id);--count_adds;if(!count_adds)++wins;}'
  assert 'me->KillAllDelayedEvents();\n            _Reset();\n            activeAddGuids.clear();\n            count_adds = 0;'in s and 'activeAddGuids.erase(summon->GetGUID()) != 1'in s
  return 'void Reset(){activeAddGuids.clear();count_adds=0;timer_check=500;}void Join(uint64 id){if(count_adds>=3||!activeAddGuids.insert(id).second)return;++count_adds;}void Die(uint64 id){if(activeAddGuids.erase(id)!=1)return;--count_adds;if(!count_adds)++wins;}'
 raise ValueError(k)
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);c=selected(e,sys.argv[3]=='baseline');(out/'Selected.inc').write_bytes(('struct Frame:Base{using Base::Base;'+c+'};').encode())
else:(out/'EighteenthNativeScripts.inc').write_bytes(('namespace EighteenthNative{'+''.join('struct N'+e['key']+':NativeBase{using NativeBase::NativeBase;'+selected(e,False)+'};'for e in plan)+'}').encode())
