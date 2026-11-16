"""Exact bonfire class extension, full-file reversal and native dispatch source."""
from pathlib import Path
import hashlib,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
PATH='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
BASE='577e7de92'
def block(s,marker):
    a=s.index(marker);i=s.index('{',a);j=i+1;depth=1
    while depth:depth+=(s[j]=='{')-(s[j]=='}');j+=1
    return s[a:j]
def verify():
    old=subprocess.check_output(['git','-C',str(ROOT),'show',BASE+':'+PATH]).decode('utf-8-sig').replace('\r\n','\n')
    new=(ROOT/PATH).read_text(encoding='utf-8-sig')
    a=new.index('        bool ownsInactiveFlag = false;');b=new.index('        void Observe(',a)
    region=new[a:b]
    assert hashlib.sha256(region.encode()).hexdigest()=='8742d842b2003718d459f1f869e7256cf5c6c85fd81b1b54be2371a31a38d783','unreviewed retirement scope'
    reverted=new[:a]+'\n'+new[b:]
    ack='            go->SetGoState(GO_STATE_ACTIVE);\n            SyncInteraction();'
    assert reverted.count(ack)==1
    reverted=reverted.replace(ack,'            go->SetGoState(GO_STATE_ACTIVE);',1)
    assert reverted==old,'another source/admission/credit/visual payload changed'
    return old,new
def generate(out,baseline=False):
    old,new=verify();out=Path(out);out.mkdir(parents=True,exist_ok=True)
    (out/'Bonfire.inc').write_text(block(old if baseline else new,'class go_mystic_bonfire : public GameObjectScript')+';')
    s=(ROOT/'src/server/game/Spells/SpellEffects.cpp').read_text(encoding='utf-8-sig');a=s.index('    if (gameObjTarget && m_spellInfo->Id == 198513');b=s.index('    // Tuure',a)
    (out/'Dispatch.inc').write_text(s[a:b])
if __name__=='__main__':
    generate(sys.argv[1],len(sys.argv)>2 and sys.argv[2]=='baseline')
    print('PASS exact consumed-bonfire scope: pinned full-file reversal; scoped instance progress->native flag,owned-only reset,post-ack sync;all original cast/credit/LOS/fog and other classes unchanged')
