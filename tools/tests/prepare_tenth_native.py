from pathlib import Path
import json,re,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);plan=json.loads((r/'tools/tests/TenthTenRepairs.json').read_text());parts=[];rows=[];force=[]
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
for e in plan:
 if not e.get('event_abort'):continue
 s=(r/e['paths'][0]).read_text(encoding='utf-8-sig')
 calls=re.findall(r'[^;\n]*m_Events\.KillAllEvents\((true|false)\);',s);assert len(calls)==e['abort_count']
 statements=re.findall(r'[^;\n]*m_Events\.KillAllEvents\((?:true|false)\);',s)
 for j,(statement,value)in enumerate(zip(statements,calls)):
  name='TenthAbort'+str(len(rows));force.append(value=='true');parts.append('static void '+name+'(Creature*actor){auto me=actor;auto summon=actor;auto l_Tuulani=actor;auto l_Nyami=actor;std::list<Creature*>list{actor};auto itr=list.begin();'+statement+'}')
  rows.append('{"'+e['key']+str(j)+'",&'+name+'}')
assert len(rows)==11 and len(set(force))==1
parts.append('struct TenthAbort{char const*name;void(*run)(Creature*);};static TenthAbort tenthAborts[]={'+','.join(rows)+'};')
if force[0]:parts.insert(0,'#define TENTH_EVENT_COUNTERFACTUAL')
else:
 for key in ['Folnuna','DreadSpray','Forge']:
  e=next(e for e in plan if e['key']==key);s=(r/e['paths'][0]).read_text(encoding='utf-8-sig');part=block(s,'class '+e['cls']+' :')+';';part=re.sub(r'(PrepareAuraScript\([^;]+\);)',r'\1\npublic:',part).replace('private:','public:');prefix=''
  names={'DreadSpray':['dread_spray_1_8'],'Forge':['SPELL_STRIKE_ANVIL_COSMETIC','SPELL_THUNDER_FORGE_CHARGE_TRIGGER']}.get(key,[])
  if key=='DreadSpray':prefix+=block(s,'enum eShaOfFearActions')+';'
  for name in names:
   m=re.search(r'\b'+name+r'\s*=\s*(\d+)',s);assert m,name;prefix+='constexpr uint32 '+name+'='+m[1]+';'
  parts.append('namespace T'+key+'{'+prefix+part+'}')
  if key=='Folnuna':
   setup=re.search(r'(?:me->SetPowerType\(POWER_ENERGY\);\s*)?me->SetMaxPower\(POWER_ENERGY, 100\);\s*me->SetPower\(POWER_ENERGY, 0\);',s[s.index('struct boss_matron_folnuna'):]).group();parts.append('static void TFolnunaSetup(Creature*me){'+setup+'}')
 # Exact queued Tectus body, including empty-list guard, exercised with real map players.
 e=next(e for e in plan if e['key']=='Tectus');s=(r/e['paths'][0]).read_text(encoding='utf-8-sig');body=re.search(r'me->RemoveFlag\(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE \| UNIT_FLAG_NOT_SELECTABLE \| UNIT_FLAG_IMMUNE_TO_PC\);\s*Map::PlayerList const& players[^}]+',s).group();value=re.search(r'SuicideNoBloodNoLogging\s*=\s*(\d+)',s)[1];parts.append('struct TTectus{Creature*me;static constexpr uint32 SuicideNoBloodNoLogging='+value+';void Finish(){'+body+'}};')
(out/'TenthNativeScripts.inc').write_text('\n'.join(parts),encoding='utf8')
