from pathlib import Path
import sys,subprocess
ROOT=Path(__file__).resolve().parents[2]
def helper(s):
 a=s.index('void CreatureAI::DespawnOnRespawn(');i=s.index('{',a);depth=1;j=i+1
 while depth:depth+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[a:j]
def verify():
 old=helper(subprocess.check_output(['git','-C',str(ROOT),'show','cf2a424da:src/server/game/AI/CreatureAI.cpp']).decode('utf-8-sig'))
 source=(ROOT/'src/server/game/AI/CreatureAI.cpp').read_text(encoding='utf-8-sig');new=helper(source)
 expected=old.replace('[this, uiTimeToDespawn]','[actor = me, uiTimeToDespawn]').replace('        me->','        actor->')
 assert new==expected and source.replace(new,old,1)==subprocess.check_output(['git','-C',str(ROOT),'show','cf2a424da:src/server/game/AI/CreatureAI.cpp']).decode('utf-8-sig').replace('\r\n','\n')
 return old,new
if __name__=='__main__':
 old,new=verify();out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);(out/'DespawnOnRespawn.inc').write_text(old if len(sys.argv)>2 and sys.argv[2]=='baseline' else new,encoding='utf-8')
 print('PASS exact helper scope: only capture/storage actor changes; original100ms,method order/respawn argument and all other CreatureAI bytes retained')
