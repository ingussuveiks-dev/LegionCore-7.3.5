"""Complete source methods/callbacks; model endpoints remain explicitly controlled."""
from pathlib import Path
import ast,json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
plan=json.loads((r/'tools/tests/FiveTen20261113Repairs.json').read_text());baseline=len(sys.argv)>2 and sys.argv[2]=='baseline'
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',plan['baseline']+':'+e['paths'][0]]).decode('utf-8-sig') if old else (r/e['paths'][0]).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text());fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='constants')
at=next(i for i,x in enumerate(fn.body)if isinstance(x,ast.Assign)and any(isinstance(y,ast.Name)and y.id=='values' for y in x.targets));fn.body.insert(at,ast.parse("texts.append((r/'src/server/game/Miscellaneous/Language.h').read_text(encoding='utf8'))").body[0])
fn.body.insert(at,ast.parse("texts.append((r/'src/server/game/AI/ScriptedAI/ScriptedGossip.h').read_text(encoding='utf8'))").body[0])
used=next(i for i,x in enumerate(fn.body)if isinstance(x,ast.Assign)and any(isinstance(y,ast.Name)and y.id=='used'for y in x.targets));fn.body.insert(used,ast.parse("code=re.sub(r'\"(?:\\\\.|[^\"\\\\])*\"','',code)").body[0]);fn.body.insert(-1,ast.parse("used.discard('M_PI')").body[0]);fn.body.insert(-1,ast.parse("used.difference_update(re.findall(r'\\b[A-Z][A-Z_0-9]+\\b(?=\\s*\\()',code))").body[0])
ns=dict(r=r,source=source,re=re);exec(compile(ast.fix_missing_locations(ast.Module(body=[fn],type_ignores=[])),'source-enums','exec'),ns)
parts=[];native=[];projections=[]
for i,e in enumerate(plan['changes']):
 path=e['path'];s=source({'paths':[path]},baseline);cls=block(s,'static bool '+e['method']+'(' if e['kind']=='command' else ('class ' if 'class '+e['cls']+' ' in s else 'struct ')+e['cls'])
 prefix='namespace Case'+str(i)+'{'
 if e['kind']=='filter':
  code=cls+';using Selected='+e['cls']+';'
 elif e['kind']=='lifetime':
  callbacks=[c['old'if baseline else'new'] for c in e['callbacks']]
  assert all(c in cls for c in callbacks),(e['key'],'complete callback changed')
  fields='' if baseline else 'std::shared_ptr<uint8> _delayedAILifetimeGuard = std::make_shared<uint8>(0);'
  fields+=''.join(re.findall(r'enum\s+\w+\s*\{[^{}]*\};',cls))
  code='struct Selected:Base{using Base::Base;'+fields+'void Start(){'+''.join(callbacks)+'}};'
  if not baseline:
   projected=[]
   for cb in callbacks:
    opening=cb.index('{',cb.index('[this,'))+1;guard_end=cb.index('return;',opening)+len('return;')
    projected.append(cb[:guard_end]+' RecordQueue('+str(i)+');});')
   base='GameObjectAI' if e['owner']=='go' else 'NullCreatureAI'
   kind='GameObject' if e['owner']=='go' else 'Creature'
   projections.append('struct Projection'+str(i)+':'+base+'{explicit Projection'+str(i)+'('+kind+'*p):'+base+'(p){}std::shared_ptr<uint8> _delayedAILifetimeGuard=std::make_shared<uint8>(0);void Start(){'+''.join(projected)+'}};')
 elif e['kind']=='once':
  code=cls+';using Selected='+e['cls']+';'
 elif e['kind']=='check_target':
  code=cls+';using Selected='+e['cls']+';'
 elif e['kind']=='aura_guard':
  code='struct Selected:SpellScript{'+block(cls,'void HandleEffectPeriodic(')+'};'
 elif e['kind']=='area_guard':
  code=cls+';using Selected='+e['cls']+';'
 elif e['kind']=='command':
  code='struct Selected{'+cls+'};'
 else:
  assert e['kind']=='snapshot'
  code=cls+';using Selected='+e['cls']+';'
  if baseline:
   expression=re.search(r'creature->AddDelayedEvent\(([^,]+),',cls)[1]
   code+='static uint64 SelectedDelay(uint32 j){return '+expression+';}'
  else:
   statement=re.search(r'uint64 delay = [^;]+;',cls)[0]
   code+='static uint64 SelectedDelay(uint32 spawned){'+statement+'return delay;}'
 constants=ns['constants']({'paths':[path]},code)
 # Camel-case native local constants in the flame core are copied as their
 # complete authored enum above. Core model constants live globally.
 constants='\n'.join(line for line in constants.splitlines() if not re.search(r'constexpr uint32 (?:UNIT_STATE_EVADE|REACT_AGGRESSIVE|CURRENT_CHANNELED_SPELL)=',line))
 parts.append(prefix+constants+code.replace(' override','')+'}')
 if e['kind']=='filter':
  native_constants='\n'.join(line for line in constants.splitlines() if not re.search(r'constexpr uint32 (?:EFFECT_|TARGET_|ROLES_|SPELL_EFFECT_)',line))
  native.append(prefix+native_constants+cls+';using Selected='+e['cls']+';}')
 if e['kind']in ('check_target','aura_guard'):
  native_constants='\n'.join(line for line in constants.splitlines() if not re.search(r'constexpr uint32 (?:EFFECT_|TARGET_|ROLES_|SPELL_EFFECT_|SPELL_AURA_|SPELL_CAST_OK|SPELL_FAILED_)',line))
  native.append(prefix+native_constants+cls+';using Selected='+e['cls']+';}')
(out/'Selected.inc').write_text('\n'.join(parts),encoding='utf8')
(out/'FiltersNative.inc').write_text('\n'.join(native),encoding='utf8')
(out/'QueuesNative.inc').write_text('\n'.join(projections),encoding='utf8')
(out/'cases.json').write_text(json.dumps([{'index':i,'key':e['key'],'kind':e['kind']}for i,e in enumerate(plan['changes'])],indent=2))
print('Extracted',len(parts),'complete source cases;baseline='+str(baseline))
