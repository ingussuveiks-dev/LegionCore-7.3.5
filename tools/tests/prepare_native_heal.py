from pathlib import Path
import sys,subprocess
repo=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
text=(repo/'src/server/scripts/Spells/spell_generic.cpp').read_text(encoding='utf-8-sig')
start=text.index('class spell_endurance_of_niuzao :');end=text.index('{',start)+1;depth=1
while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
(out/'NativeNiuzao.inc').write_text(text[start:end]+';\n',encoding='utf8')
text=subprocess.check_output(['git','-C',str(repo),'show','cbd047ef4:src/server/game/Entities/Unit/Unit.cpp']).decode()
start=text.index('int32 Unit::DealHeal(');end=text.index('{',start)+1;depth=1
while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
(out/'NativeHealApiBefore.inc').write_text(text[start:end].replace('Unit::DealHeal(','BeforeDealHeal(',1)+'\n',encoding='utf8')
