"""Full selected healing classes; native data and dispatcher checks are separate."""
import json,re
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block

def prepare(out,mode):
 rows=[e for e in PLAN['changes']if e['batch']==8];assert len(rows)==10;require_fixed([e['key']for e in rows]);text=''
 for e in rows:
  ed=e['edits'][e.get('class_edit_index',0)];body=ed['old'if mode=='baseline'else'new']
  assert e['case']!='azerite'
  text+=body+';\n'
 excluded={'NULL','EFFECT_0','EFFECT_1','EFFECT_2','EFFECT_3','EFFECT_4','IN_MILLISECONDS','AURA_REMOVE_BY_EXPIRE','AURA_REMOVE_BY_ENEMY_SPELL','SPELL_EFFECT_DUMMY','SPELL_EFFECT_HEAL','SPELL_EFFECT_WEAPON_PERCENT_DAMAGE','TARGET_SRC_CASTER','TARGET_GAMEOBJECT_SRC_AREA','TARGET_UNIT_SRC_AREA_ENTRY','SPELL_AURA_SCHOOL_ABSORB','SPELL_AURA_PERIODIC_DUMMY','AURA_EFFECT_HANDLE_REAL'}
 labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',re.sub(r'//[^\n]*|/\*.*?\*/',' ',text,flags=re.S)))-excluded-{'RANGED_ATTACK','QUEST_STATUS_INCOMPLETE'}
 # Symbolic class labels are explicit; actual matching26972 values/hooks are native checked.
 text='enum EighthLabels{'+','.join(k+'='+str(100+i)for i,k in enumerate(sorted(labels)))+'};\n'+text
 out.mkdir(parents=True,exist_ok=True);(out/'Selected.inc').write_text(text,encoding='utf8');(out/'cases.json').write_text(json.dumps([e['key']for e in rows]));print('Extracted ten complete production class/create-hook scopes;external spell/instance endpoints controlled')
