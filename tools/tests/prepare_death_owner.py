"""Extract full production death methods; instance/quest endpoints are controlled.
Native extraction projects only each actual source owner/instance getter.
"""
from pathlib import Path
import json,re,subprocess

def generate(plan_path,base,out,case=None,baseline=False):
 r=Path(__file__).resolve().parents[2];out=Path(out);out.mkdir(parents=True,exist_ok=True);plan=json.loads(Path(plan_path).read_text())
 for e in plan:
  source=subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf-8-sig')if baseline else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
  assert e['edits'][0]['old'if baseline else'new']in source,e['key']
 if case is None:
  text=''
  for e in plan:
   assert e['selector']in e['edits'][0]['new']
   signature='InstanceScript* Resolve(Creature* me,Unit* killer)'if e['kind']=='victim_instance'else'Player* Resolve(Unit* killer)'
   text+='struct '+e['key']+'{static '+signature+'{return '+e['selector']+';}};\n'
  (out/'FortySixthNativeSelectors.inc').write_text(text,encoding='utf8');return
 methods=[e['edits'][0]['old'if baseline else'new']for e in plan]
 excluded={'NULL','AI','TYPEID_PLAYER','TYPEID_UNIT','TEAM_ALLIANCE','TEAM_HORDE','QUEST_STATUS_INCOMPLETE'}
 labels=sorted(set(re.findall(r'\b[A-Z][A-Z_0-9]+\b','\n'.join(methods)))-excluded|{'SH_AELMAR','ActionAllianceRecruit','ActionHordeRecruit','NeutralKorlokTheOgreKing'})
 # These are controlled endpoint labels,not substitute production spell IDs.
 # Exact source reversal and compiled production callbacks preserve real IDs.
 text='enum FixtureLabels{'+','.join(label+'='+str(100+i)for i,label in enumerate(labels))+'};\nusing InstanceScript=Instance;\n'
 for i,method in enumerate(methods):text+='struct Frame'+str(i)+':Base{using Base::Base;static constexpr uint32 cleanupAuraCount='+str(method.count('DoRemoveAurasDueToSpellOnPlayers('))+';'+method+'};\n'
 text+='#define CASE_DISPATCH '+''.join('case '+str(i)+':Run<Frame'+str(i)+'>(which);break;'for i in range(10))+'\n'
 (out/'Selected.inc').write_text(text,encoding='utf8')
