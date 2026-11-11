"""Project only production capture/weak admission onto a controlled payload.
Actual payload bytes/parameters and Release closures are checked separately.
This does not execute or certify the encounter body.
"""
from pathlib import Path
import json,subprocess,sys
from lifetime_queue_ownership import queue_owner

def generate(plan_path,base,out,case=None,baseline=False,native_name=None):
 r=Path(__file__).resolve().parents[2];out=Path(out);out.mkdir(parents=True,exist_ok=True);plan=json.loads(Path(plan_path).read_text(encoding='utf8'))
 def emit(e,native):
  source=subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if baseline else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
  for ed in e['edits']:assert ed['old'if baseline else'new']in source,(e['key'],'complete production class')
  for cb in e['callbacks']:assert cb['old'if baseline else'new']in source,(e['key'],'complete registration')
  for cb in e['callbacks']:assert queue_owner(e,source,cb)=='Unit',(e['key'],'AI-owned queue cannot use a Unit-queue projection')
  decl='struct Frame:NullCreatureAI{Creature*peer;Frame(Creature*c,Creature*p):NullCreatureAI(c),peer(p){}'if native else'struct Frame{Creature*me;Creature*peer;Frame(Creature*c,Creature*p):me(c),peer(p){}'
  if not baseline:decl+=e['member']
  decl+='void Queue(uint32 index){switch(index){'
  for i,cb in enumerate(e['callbacks']):
   capture='this'+(', '+e['guard_init']if not baseline else'');guard=e['guard']if not baseline else'';owner='me'if cb['queue_owner']=='me'else'peer';queue='AddDelayedCombat'if cb['queue']=='combat'else'AddDelayedEvent';effect=f'Observe::Mark(me,{i+1});'if native else'me->Touch();'
   decl+=f'case {i}:{{{owner}->{queue}(123,[{capture}](){{{guard}{effect}}});break;}}'
  decl+='default:throw std::runtime_error("invalid admission case");}}};'
  decl+='static bool const combat[]={'+','.join('true'if c['queue']=='combat'else'false'for c in e['callbacks'])+'};'
  decl+='static bool const other[]={'+','.join('false'if c['queue_owner']=='me'else'true'for c in e['callbacks'])+'};'
  return decl
 if case:
  e=next(e for e in plan if e['key']==case)
  if e.get('withdrawn'):raise ValueError('Withdrawn case: '+e['withdrawal_reason'])
  (out/'Selected.inc').write_bytes(emit(e,False).encode())
 else:(out/(native_name+'Scripts.inc')).write_bytes(('namespace '+native_name+'{'+''.join('namespace '+e['key']+'{'+emit(e,True)+'}'for e in plan if not e.get('withdrawn'))+'}').encode())
