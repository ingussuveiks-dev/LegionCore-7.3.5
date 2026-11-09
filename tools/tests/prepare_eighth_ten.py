from pathlib import Path
import sys,json,subprocess,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);key=sys.argv[2];baseline=sys.argv[3]=='baseline';plan=json.loads((r/'tools/tests/EighthTenRepairs.json').read_text());e=next(e for e in plan if e['key']==key)
def source(p):return subprocess.check_output(['git','-C',str(r),'show','f884e6321:'+p]).decode('utf8') if baseline else (r/p).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
s=source(e['paths'][0]);constants='';selected=''
if key=='BossDeath':selected=block(s,'void BossAI::_JustDied()')
elif key=='Hyjal':selected=block(s,'class player_hyjal_protectors_random :')+';'
else:
 scope=block(s,'class '+e['cls']+' :');argument='attacker'if key=='Baleful'else'killer';body=block(scope,'void JustDied(Unit* '+argument+')');selected='struct DeathFrame:BaseAI{using BaseAI::BaseAI;'+body+'};'
 if key in ['Webbed','WebbedPool']:constants+=re.search(r'uint32 const possibleSpawns\[32\]\s*=\s*\{[^}]+\};',s).group()+'\n'
 if key=='Baleful':constants+=block(scope,'    enum data')+';\n'
 if key=='Sentry':constants+=block(s,'enum eSentry')+';\n'
 if key=='Torloth':constants+='#define QUEST_BATTLE_OF_THE_CRIMSON_WATCH 10781\n'
 if key=='SilverHand':
  constants+='\n'.join(line for line in source('src/server/scripts/EasternKingdoms/Stratholme/stratholme.h').splitlines()if line.startswith('#define TYPE_SH_'))+'\n'
  constants+='\n'.join(line for line in s.splitlines()if line.startswith('#define SH_'))+'\n'
 if key=='BloodQueen':
  constants+=block(s,'enum Texts')+';\n'+block(s,'enum Spells')+';\n'
  header=source('src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h')
  for name in ['DATA_BLOOD_QUICKENING_STATE','NPC_INFILTRATOR_MINCHAR_BQ']:
   value=re.search(name+r'\s*=\s*(\d+)',header);assert value,name;constants+='constexpr uint32 '+name+'='+value[1]+';\n'
(out/'Constants.inc').write_text(constants,encoding='utf8');(out/'Selected.inc').write_text(selected,encoding='utf8')
