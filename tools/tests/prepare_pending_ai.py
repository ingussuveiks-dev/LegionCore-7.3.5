"""Current admissions are checked directly, outside historical retained views."""
from pathlib import Path
import sys,json,subprocess
from verify_pending_ai_scope import verify,plan,root
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);case=sys.argv[2] if len(sys.argv)>2 else None;baseline=len(sys.argv)>3 and sys.argv[3]=='baseline'
if not baseline:verify()
def emit(e,native):
 source=subprocess.check_output(['git','-C',str(root),'show','cf1182f05:'+e['paths'][0]]).decode('utf-8-sig') if baseline else(root/e['paths'][0]).read_text(encoding='utf-8-sig')
 for cb in e['callbacks']:assert cb['old' if baseline else 'new'] in source
 s='struct Frame:NullCreatureAI{Creature*peer;Frame(Creature*c,Creature*p):NullCreatureAI(c),peer(p){}' if native else 'struct Frame{Creature*me,*peer;Frame(Creature*c,Creature*p):me(c),peer(p){}'
 if not baseline:s+=e['member']
 s+='void Queue(uint32 index){switch(index){'
 for i,cb in enumerate(e['callbacks']):
  capture='this'+(', '+e['guard_init'] if not baseline else '');guard=e['guard'] if not baseline else '';effect=f'Observe::Mark(me,{i+1});' if native else 'me->Touch();'
  s+=f'case {i}:{{me->AddDelayedEvent(123,[{capture}](){{{guard}{effect}}});break;}}'
 return s+'default:throw std::runtime_error("invalid admission case");}}};static bool const combat[]={'+','.join('false' for _ in e['callbacks'])+'};static bool const other[]={'+','.join('false' for _ in e['callbacks'])+'};'
if case:(out/'Selected.inc').write_text(emit(next(e for e in plan if e['key']==case),False),encoding='utf-8')
else:(out/'PendingAIScripts.inc').write_text('namespace PendingAI{'+''.join('namespace '+e['key']+'{'+emit(e,True)+'}' for e in plan)+'}',encoding='utf-8')
