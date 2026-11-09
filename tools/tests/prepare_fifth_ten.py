"""Extract complete changed methods/event branches; endpoints are modeled separately."""
from pathlib import Path
import json,subprocess,sys,re
r=Path(__file__).resolve().parents[2]
out=Path(sys.argv[1]);case=sys.argv[2];baseline=len(sys.argv)>3 and sys.argv[3]=='baseline'
plan=json.loads((r/'tools/tests/FifthTenRepairs.json').read_text());e=next(x for x in plan if x['key']==case)
def source(path):return subprocess.check_output(['git','-C',str(r),'show','bc19ca10b:'+path]).decode('utf8')if baseline else(r/path).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
s=source(e['path']);selected=''
if case=='Jump':
 h=source(e['extra'][0]);a=h.index('        void MoveJump(Position const&');b=h.index('        void MoveFall',a)
 declarations=h[a:b]
 if not baseline:declarations='void MoveJump(float x, float y, float z, float speedXY, float speedZ);\n'+declarations
 selected='struct MotionMaster{'+declarations+'};\n'
 selected+='void MotionMaster::MoveJump(float x,float y,float z,float o,float xy,float vertical,uint32 id,bool facing,JumpArrivalCastArgs const*,Movement::SpellEffectExtraData const*){jump={x,y,z,o,xy,vertical,id,facing};}\n'
 if not baseline:selected+=block(s,'void MotionMaster::MoveJump(float x, float y, float z, float speedXY, float speedZ)')
elif case=='Harjatan':
 a=s.index('                    case EVENT_HATCHING:');b=s.index('                    case EVENT_HATCHING_CLEAN_UP:',a)
 selected='void Probe::Run(){switch(EVENT_HATCHING){'+s[a:b]+'}}'
elif case=='Antorus':
 a=s.index('            if (timerIntroEvent2 <= diff)');b=s.index('            if (timerConstEvent <= diff)',a)
 selected='void Probe::Run(){uint32 diff=5000;'+s[a:b]+'}'
elif case=='Books':selected='void Probe::Run(){'+block(s,'                        if (GetDifficultyID() != DIFFICULTY_HEROIC)')+'}'
elif case=='Zao':
 a=s.index('            case EVENT_ZAO_ATTACK:');b=s.index('                case EVENT_3:',a)
 selected='void Probe::Run(){switch(EVENT_ZAO_ATTACK){'+s[a:b]+'}}'
elif case=='Cthun':selected=block(s,'        Unit* SelectRandomNotStomach()').replace('Unit* SelectRandomNotStomach()','Unit* Probe::SelectRandomNotStomach()')
elif case=='Rain':selected=block(s,'        class spell_sha_healing_rain_SpellScript :')+';'
elif case=='Devotion':selected=block(s,'struct spell_pal_at_devotion_aura :')+';'
else:selected=block(s,'class '+e['cls']+' :')+';'
(out/'Selected.inc').write_text(selected,encoding='utf8')
# The exact local API declarations define the overload boundary.
pos=(r/'src/server/game/Entities/Object/Position.h').read_text();a=pos.index('    Position(float');b=pos.index('\n\n',a)
(out/'PositionCtor.inc').write_text(pos[a:b],encoding='utf8')
