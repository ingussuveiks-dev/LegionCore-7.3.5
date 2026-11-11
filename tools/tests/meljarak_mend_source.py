"""Explicit later Mending changes,kept outside the prior shared-death scope."""
from pathlib import Path
import subprocess
FORMULA='''                    uint32 const percentage = uint32(std::min(100.0f, std::max(0.0f,
                        spell->GetEffect(EFFECT_0, GetDifficultyID())->BasePoints)));
                    uint64 const maximum = me->GetMaxHealth();
                    uint64 const modhealth = (maximum / 100) * percentage +
                        ((maximum % 100) * percentage) / 100;'''
LEGACY='                    uint64 modhealth = me->GetMaxHealth()/4;'
def without_mending(source):
    assert source.count(FORMULA)==1
    source=source.replace(FORMULA,LEGACY,1)
    start=source.index('// Heroic122147 retains its native source-area-entry selection.')
    opening=source.index('{',source.index('class spell_meljarak_mending_target',start));end=opening+1;depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    assert source[end:end+3]==';\n\n'
    source=source[:start]+source[end+3:]
    line='    new spell_meljarak_mending_target();\n';assert source.count(line)==1
    return source.replace(line,'',1)
if __name__=='__main__':
    repo=Path(__file__).resolve().parents[2];path='src/server/scripts/Pandaria/HeartFear/boss_lord_meljarak.cpp'
    source=(repo/path).read_text(encoding='utf-8-sig');old=subprocess.check_output(['git','-C',str(repo),'show','9a0af9317:'+path]).decode().replace('\r\n','\n')
    assert without_mending(source)==old,'Unrelated Meljarak combat,damage/credit,CC,respawn or native cast changed'
    print('PASS exact Mending source scope:profile amount,new encounter-only area filter and registration;all previous encounter bodies unchanged')
