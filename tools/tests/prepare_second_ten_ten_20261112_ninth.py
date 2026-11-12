from pathlib import Path
import sys,re,json
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[r for r in PLAN['changes']if r['batch']==9];assert len(rows)==10
parts=[];provenance=[]
for i,row in enumerate(rows[:9]):
 source=row['edits'][0]['old'if mode=='baseline'else'new'];bodies=[block(source,r'void '+m+r'\(').replace(' override','')for m in row['methods']];parts.append('struct NinthClock'+str(i)+':ClockAI{using ClockAI::ClockAI;'+row['fixture_fields']+'\n'+'\n'.join(bodies)+'\n};');provenance.append(dict(key=row['key'],scope='Complete production Reset/EnterCombat methods, plus complete Felmyst EnterPhase and Unga Reset/DoAction/UpdateAI; constructors and other actor/instance/spatial endpoints controlled. Native EventMap separately used.'))
source=rows[9]['edits'][0]['old'if mode=='baseline'else'new'];arms=[]
for label in ['EVENT_SHADOWBLAZE','EVENT_TAIL_LASH']:
 start=source.index('case '+label+':');end=source.index('break;',start)+len('break;');arms.append(source[start:end])
parts.append('struct NinthClock9:ClockAI{using ClockAI::ClockAI;void Dispatch(uint32 eventId){switch(eventId){'+'\n'.join(arms)+'}}};');provenance.append(dict(key=rows[9]['key'],scope='Two complete production Nefarian event arms are explicit projections; full phase controller and creature endpoints are not modeled as full native combat.'))
text='\n'.join(parts);labels=set(re.findall(r'\b(?:EVENT|SPELL|SAY|DATA|NPC|PHASE|AURA|MOB|TEXT)_[A-Za-z_0-9]+\b|\b[A-Z][A-Z_0-9]+\b',re.sub(r'//[^\n]*|/\*.*?\*/',' ',text,flags=re.S)))|{r['stale_event']for r in rows[:8]}|{'IN_PROGRESS','DONE','NPC_SETTHIK_WINDBLADE','NPC_KORTHIK_REAVER'}
fixed={'IN_MILLISECONDS':1000,'IN_PROGRESS':1,'DONE':2}
for name in sorted(labels-set(fixed)-{'NULL'}):fixed[name]=len(fixed)+100
(out/'NinthClockLabels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n');(out/'NinthSelected.inc').write_text(text);(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));(out/'fixture-scope.json').write_text(json.dumps(provenance,indent=2));print('Prepared complete reset/reentry callbacks and two explicitly projected Nefarian event arms; native EventMap, bindings, linked code and client scopes remain separate.')
