"""Pin the small eligibility change and retain the native dispatch boundaries."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
path = 'src/server/game/Entities/Creature/Creature.h'
old = subprocess.check_output(['git', '-C', str(root), 'show', '5cc72f230:' + path]).decode('utf8').replace('\r\n', '\n')
expected = old.replace('#include "Unit.h"', '#include "Unit.h"\n#include <limits>', 1)
expected = expected.replace('void LowerPlayerDamageReq(uint32 unDamage)', 'void LowerPlayerDamageReq(uint64 unDamage)', 1)
expected = expected.replace('            m_PlayerDamageReq += unDamage;', '''            // This core accumulates damage and tests for a positive value.
            // Preserve eligibility when either a hit or the sum exceeds uint32.
            uint64 const remaining = std::numeric_limits<uint32>::max() - m_PlayerDamageReq;
            m_PlayerDamageReq += uint32(std::min(unDamage, remaining));''', 1)
assert (root / path).read_text(encoding='utf8') == expected
for path in ['src/server/game/Entities/Unit/Unit.cpp', 'src/server/game/Entities/Creature/Creature.cpp',
             'src/server/game/Entities/Player/Player.cpp', 'src/server/game/Spells/Spell.cpp',
             'src/server/game/Entities/Player/KillRewarder.cpp']:
    before = subprocess.check_output(['git', '-C', str(root), 'show', '5cc72f230:' + path]).decode('utf8').replace('\r\n', '\n')
    assert (root / path).read_text(encoding='utf8') == before, path
print('PASS native loot scope: only full-width input and saturation; counter layout, reset, positive predicate, native post-scaling/player-controlled damage dispatch, pet/guardian owner/group reward and corpse-loot paths unchanged. This is source preservation, not a client/group combat traversal.')
