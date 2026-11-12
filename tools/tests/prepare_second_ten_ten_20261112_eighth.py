from pathlib import Path
import sys,re,json,subprocess
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[r for r in PLAN['changes']if r['batch']==8];assert len(rows)==10
parts=[];provenance=[]
for i,row in enumerate(rows[:8]):
 source=row['edits'][0]['old'if mode=='baseline'else'new'];fields=row['fixture_fields'];base='HighSeasClockBase'if i==4 else'ClockAI'
 if i==4 and mode=='baseline':fields+=' EventMap events;'
 bodies=[block(source,r'void '+method+r'\(').replace(' override','')for method in row['methods']]
 parts.append('struct EightClock'+str(i)+':'+base+'{using '+base+'::'+base+';'+fields+'\n'+'\n'.join(bodies)+'\n};')
 provenance.append(dict(key=row['key'],scope='Complete production Reset/EnterCombat/UpdateAI callbacks named in plan; constructor, extra hooks and creature/instance/spatial endpoints controlled. Shared HighSeas ExecuteTargetEvent is extracted in full.'))
text='\n'.join(parts);helper=block((ROOT/'src/server/scripts/Scenario/BattleOnTheHighSeas/battle_on_the_high_seas.cpp').read_text(),r'void ExecuteTargetEvent\(')
helper=helper.replace('HighSeasTargetPriority priority','uint32 priority')
labels=set(re.findall(r'\b(?:EVENT|SPELL|SAY|DATA|PRIORITY)_[A-Za-z_0-9]+\b|\b[A-Z][A-Z_0-9]+\b',re.sub(r'//[^\n]*|/\*.*?\*/',' ',text+helper,flags=re.S)))|{'IN_PROGRESS','DONE','SPELL_FIERY_SLASH','SPELL_ARMAGEDDON','SPELL_COUNTER_SHOT','SPELL_PROTECTIVE_FENZY'}
fixed={'IN_MILLISECONDS':1000,'IN_PROGRESS':1,'DONE':2}
for name in sorted(labels-set(fixed)-{'NULL'}):fixed[name]=len(fixed)+100
(out/'ClockLabels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n');(out/'SelectedClock.inc').write_text(text);(out/'HighSeasClockHelper.inc').write_text(helper)
auras='\n'.join(row['edits'][0]['old'if mode=='baseline'else'new']+';'for row in rows[8:]);(out/'SelectedAura.inc').write_text(auras)
(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));(out/'fixture-scope.json').write_text(json.dumps(provenance,indent=2));print('Prepared complete eight combat callback sequences and full two AuraScript classes; controlled endpoints and actual native EventMap/core are separate scopes.')
