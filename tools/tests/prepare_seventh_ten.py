from pathlib import Path
import sys,json,subprocess,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);key=sys.argv[2];baseline=sys.argv[3]=='baseline'
plan=json.loads((r/'tools/tests/SeventhTenRepairs.json').read_text());e=next(e for e in plan if e['key']==key)
def source(p):return subprocess.check_output(['git','-C',str(r),'show','92ccd64d1:'+p]).decode('utf8') if baseline else (r/p).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
s=source(e['paths'][0]);selected=''
if key=='ArtifactCondition':
 a=s.index('        case CONDITION_ARTIFACT_POWER:');b=s.index('        case CONDITION_SPEC_ID:',a)
 selected='struct ConditionFrame{uint32 ConditionValue1=0,ConditionValue2=0,ConditionValue3=100;bool Meets(Unit*object){bool condMeets=false;switch(57){'+s[a:b]+'}return condMeets;}};'
elif key=='Transport':
 decl=next(line.strip() for line in s.splitlines() if 'ObjectGuid::LowType transLowGUID ='in line and 'fields[f_transguid]'in line)
 body=block(s,'    else if (transLowGUID)').replace('else if','if',1)
 selected='struct TransportFrame{uint32 mapId=1;bool bind=false;Field fields[1];ObjectGuid guid{1099511627993};Movement m_movementInfo;void RelocateToHomebind(){bind=true;}bool Load(){'+decl+'\n'+body+'\nreturn transLowGUID!=0;}};'
else:
 scope=block(s,'class '+e['cls']+' :');method=re.search(r'\s+void JustDied\(Unit\* killer\) override',scope);assert method
 body=block(scope,method.group().strip())
 if 'enum data' in scope:selected+=block(scope,'        enum data')+';\n'
 if key=='Netherweb':
  selected+='constexpr uint32 QUEST_TARGET=22459;\n'+re.search(r'const uint32 netherwebVictims\[6\]\s*=\s*\{[^}]+\};',s).group()+'\n'
 if key=='Nerubar':selected+='constexpr uint32 WARSONG_PEON=25270;\n'+re.search(r'const uint32 nerubarVictims\[3\]\s*=\s*\{[^}]+\};',s).group()+'\n'
 if key=='Talbot':selected+=block(s,'enum eThassarian')+';\n'
 selected+='struct DeathFrame:BaseAI{using BaseAI::BaseAI;ObjectGuid TyrannaGuid{1},LeryssaGUID{2},ArlosGUID{3};'+body+'};'
(out/'Selected.inc').write_text(selected,encoding='utf8')
