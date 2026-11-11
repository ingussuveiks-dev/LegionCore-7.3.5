from pathlib import Path
import subprocess,sys

BASE='2494239c1e62513dd355b358959eb45b4d78f1a3'
PATH='src/server/scripts/Legion/VaultoftheWardens/boss_cordana.cpp'
def block(source,signature):
    start=source.index(signature);first=source.index('{',start);depth=1;end=first+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

def generate(out,baseline=False):
    repo=Path(__file__).resolve().parents[2];out=Path(out);out.mkdir(parents=True,exist_ok=True)
    source=subprocess.check_output(['git','-C',str(repo),'show',BASE+':'+PATH]).decode()if baseline else(repo/PATH).read_text(encoding='utf-8-sig')
    coords=block(source,'std::map<uint8, std::pair<Position, Position> > creeping_walls')+';'
    method=block(source,'void SummonCreepings(uint8 wave)')
    # Observe only the native spawn boundary;the entire scheduling/geometry
    # method and actual Unit queue/Position types execute in the native host.
    method=method.replace('urand(','CordanaRand(').replace('me->SummonCreature(','CordanaRequest(')
    text=coords+'\nstruct WallsFrame{Creature*me;Difficulty difficulty;std::shared_ptr<uint8> _delayedAILifetimeGuard=std::make_shared<uint8>(0);WallsFrame(Creature*c,Difficulty d):me(c),difficulty(d){}Difficulty GetDifficultyID(){return difficulty;}void Talk(uint8){++wallTalks;}'+method+'};\n'
    (out/'CordanaWalls.inc').write_text(text,encoding='utf8')

if __name__=='__main__':generate(sys.argv[1],len(sys.argv)>2 and sys.argv[2]=='baseline')
