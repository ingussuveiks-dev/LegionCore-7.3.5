from pathlib import Path
import sys
from warswords_runestone_scope import verify
verify();R=Path(__file__).resolve().parents[2];out=Path(sys.argv[1])
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;depth=1
 while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
s=(R/'src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp').read_text(encoding='utf-8-sig')
(out/'RunestoneAI.inc').write_text(block(s,'class go_prison_runestone : public GameObjectScript')+';',encoding='utf-8')
s=(R/'src/server/game/Spells/SpellEffects.cpp').read_text(encoding='utf-8-sig')
a=s.index('    if (gameObjTarget && m_spellInfo->Id == 8386');b=s.index("    // Tuure's native goober bypasses",a)
assert s.index('SpellCastResult res = CanOpenLock(',s.index('void Spell::EffectOpenLock('))<a
(out/'RunestoneDispatch.inc').write_text(s[a:b],encoding='utf-8')
print('Extracted complete production rune AI and its actual post-lock dispatch')
