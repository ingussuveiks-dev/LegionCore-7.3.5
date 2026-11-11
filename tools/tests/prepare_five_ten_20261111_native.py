from pathlib import Path
import json,re,sys
from five_ten_20261111_scope import ROOT,PLAN,require_fixed
require_fixed();out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
text=''
for key,var in [('RhyolithPhaseHealth','_health'),('MurmurResetHealth','hp')]:
 row=next(e for e in PLAN['changes']if e['key']==key)
 text+='struct '+key+'Native:NullCreatureAI{using NullCreatureAI::NullCreatureAI;uint64 Amount(){'+row['edits'][0]['new']+'return '+var+';}};\n'
row=next(e for e in PLAN['changes']if e['key']=='MeatballDeathQueueLifetime');body=row['edits'][0]['new']
member=re.search(r'std::shared_ptr<uint8> _delayedAILifetimeGuard[^;]+;',body)[0]
init=re.search(r'aiLifetimeGuard = std::weak_ptr<uint8>\(_delayedAILifetimeGuard\)',body)[0]
guard=re.search(r'if \(aiLifetimeGuard.expired\(\)\) return;',body)[0]
assert body.count(init)==body.count(guard)==2
text+='struct MeatballNative:NullCreatureAI{MeatballNative(Creature*c,Creature*):NullCreatureAI(c){}'+member+'void Queue(uint32 index){me->AddDelayedEvent(123,[this,'+init+']{'+guard+'Observe::Mark(me);});}};\n'
text+='static std::vector<std::pair<uint32,std::string>>const ExpectedBindings={\n'
bindings=set()
for row in PLAN['changes']:
 for pair in row.get('native_bindings',[]):bindings.add((pair['entry'],pair['script']))
assert len(bindings)>30,len(bindings)
text+=',\n'.join('{'+str(entry)+',"'+script+'"}'for entry,script in sorted(bindings))+'};\n'
text+='struct NativeDataBinding{char const*kind;uint32 entry;char const*script;};static NativeDataBinding const ExpectedDataBindings[]={'+','.join('{"'+e['kind']+'",'+str(e['entry'])+',"'+e['script']+'"}'for e in PLAN['data_bindings'])+'};\n'
row=next(e for e in PLAN['changes']if e['key']=='ShaDoubtHealAmount')
text+='static void NativeShaHeal(Creature*me,Unit*creature,SpellInfo const*spellInfo){'+row['edits'][0]['new']+'}\n'
(out/'FiveTen20261111Native.inc').write_text(text,encoding='utf8')
print('Prepared native source projections and',len(bindings),'exact read-only creature bindings')
