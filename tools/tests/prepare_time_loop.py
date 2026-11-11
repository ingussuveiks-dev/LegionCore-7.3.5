from pathlib import Path
import subprocess,sys
repo=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
path='src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_ultraxion.cpp'
source=subprocess.check_output(['git','-C',str(repo),'show',('4d61d0dfb' if sys.argv[2]=='heal-baseline' else 'cbd047ef4')+':'+path]).decode() if len(sys.argv)>2 and sys.argv[2] in ('baseline','heal-baseline') else (repo/path).read_text()
start=source.index('class spell_ultraxion_time_loop :');end=source.index('{',start)+1;depth=1
while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
(out/'TimeLoopScript.inc').write_text(source[start:end]+';\n',encoding='utf8')
