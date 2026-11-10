"""Pinned pre-fix reset-state controls; selected production assignments, no uninitialized read."""
from pathlib import Path
import json,subprocess,sys,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/SeventeenthTenRepairs.json').read_text());base='4a7cae7e4'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def selected(e,old):
 s=subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8') if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
 c=block(s,('struct ' if 'struct '+e['cls'] in s else 'class ')+e['cls']);reset=block(c,'void Reset(')
 fields=''.join('uint32 '+f+'=0xCDCDCDCD;' for f in e['fields']);assign=''
 for f in e['fields']:
  m=re.search(r'\b'+f+r'\s*=\s*(?:\d+|true|false);',reset)
  if m:assign+=m.group()
 return fields+'void Reset()'+('{'+assign+'}')
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);old=sys.argv[3]=='baseline';c=selected(e,old)
 (out/'Selected.inc').write_bytes(('struct Frame{'+c+'};').encode());checks=' && '.join('f.'+k+'=='+str(v) for k,v in e['fields'].items());poison=''.join('f.'+k+'=0xCDCDCDCD;' for k in e['fields']);(out/'Check.inc').write_bytes(('Frame f;f.Reset();bool first='+checks+';'+poison+'f.Reset();bool again='+checks+';\n#ifdef COUNTERFACTUAL\nRequire(!first&&!again,"pinned old reset fails both first-use and retry contract");\n#else\nRequire(first&&again,"first reset and retry restore authored state");\n#endif\n').encode())
else:
 parts=[]
 for e in plan:
  c=selected(e,False).replace('void Reset()','void Reset() override')
  parts.append('struct N'+e['key']+':ScriptedAI{using ScriptedAI::ScriptedAI;'+c+'};')
 (out/'SeventeenthNativeScripts.inc').write_bytes(('namespace SeventeenthNative{'+''.join(parts)+'}').encode())
