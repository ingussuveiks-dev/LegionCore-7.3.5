from pathlib import Path
import subprocess,sys

def generate(out):
    repo=Path(__file__).resolve().parents[2];out=Path(out);out.mkdir(parents=True,exist_ok=True)
    source=subprocess.check_output(['git','-C',str(repo),'show','9a0af9317:src/server/game/Spells/SpellScript.cpp']).decode('utf8')
    start=source.index('bool SpellScript::TargetHook::CheckEffect(');end=source.index('{',start)+1;depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    (out/'TargetHookBefore.inc').write_text(source[start:end].replace('SpellScript::TargetHook::','',1),encoding='utf8')

if __name__=='__main__':generate(sys.argv[1])
