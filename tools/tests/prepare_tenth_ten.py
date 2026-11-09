from pathlib import Path
import sys,json,re,subprocess
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);key=sys.argv[2];baseline=sys.argv[3]=='baseline';plan=json.loads((r/'tools/tests/TenthTenRepairs.json').read_text());e=next(e for e in plan if e['key']==key)
def source(p):return subprocess.check_output(['git','-C',str(r),'show','628367eab:'+Path(p).as_posix()]).decode('utf8')if baseline else(r/p).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
s=source(e['paths'][0]);selected='';constants=''
if e.get('event_abort'):
 header=(r/'src/common/Utilities/EventProcessor.h').read_text();cpp=(r/'src/common/Utilities/EventProcessor.cpp').read_text();event=block(header,'class TC_COMMON_API BasicEvent').replace('TC_COMMON_API ','')+';typedef std::multimap<uint64,BasicEvent*>EventList;'+block(header,'class TC_COMMON_API EventProcessor').replace('TC_COMMON_API ','')+';'+cpp[cpp.index('BasicEvent::BasicEvent()'):];(out/'EventProcessor.inc').write_text(event)
 calls=re.findall(r'[^;\n]*m_Events\.KillAllEvents\((?:true|false)\);',s);assert len(calls)==e['abort_count'];parts=[]
 for j,call in enumerate(calls):parts.append('static void Selected'+str(j)+'(Creature*actor){auto me=actor;auto summon=actor;auto l_Tuulani=actor;auto l_Nyami=actor;std::list<Creature*>list{actor};auto itr=list.begin();'+call+'}')
 parts.append('static void(*selected[])(Creature*)={'+','.join('&Selected'+str(j)for j in range(len(calls)))+'};');selected='\n'.join(parts)
elif key=='Tectus':
 start=s.index('me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);',s.index('AddDelayedEvent(8 * IN_MILLISECONDS'));a=s.index('});',start);body=s[start:a];selected='struct Selected: AuraScript{using AuraScript::AuraScript;void Finish(){'+body+'}};';constants='constexpr uint32 SuicideNoBloodNoLogging='+re.search(r'SuicideNoBloodNoLogging\s*=\s*(\d+)',s)[1]+';'
else:
 selected=block(s,'class '+e['cls']+' :')+';';selected=re.sub(r'(PrepareAuraScript\([^;]+\);)',r'\1\npublic:',selected).replace('private:','public:')
 names={'DreadSpray':['dread_spray_1_8'],'Forge':['SPELL_STRIKE_ANVIL_COSMETIC','SPELL_THUNDER_FORGE_CHARGE_TRIGGER']}.get(key,[])
 if key=='DreadSpray':constants+=block(s,'enum eShaOfFearActions')+';'
 for name in names:
  m=re.search(r'\b'+name+r'\s*=\s*(\d+)',s);assert m,name;constants+='constexpr uint32 '+name+'='+m[1]+';'
 selected+='\nusing Selected='+e['cls']+';'
 if key=='Folnuna':
  setup=re.search(r'(?:me->SetPowerType\(POWER_ENERGY\);\s*)?me->SetMaxPower\(POWER_ENERGY, 100\);\s*me->SetPower\(POWER_ENERGY, 0\);',s[s.index('struct boss_matron_folnuna'):]).group();selected+='static void SelectedSetup(Creature*me){'+setup+'}'
(out/'Constants.inc').write_text(constants);(out/'Selected.inc').write_text(selected,encoding='utf8')
