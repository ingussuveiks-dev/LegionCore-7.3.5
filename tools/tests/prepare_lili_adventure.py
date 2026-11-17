from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1])
source = (root / 'src/server/scripts/Legion/ClassHalls/lili_adventure.cpp').read_text(encoding='utf-8-sig')
start = source.index('namespace LiliAdventure')
end = source.index('\nclass npc_lili_adventure :')
production = source[start:end]
for forbidden in ('KilledMonsterCredit', 'RewardPlayerAndGroup', 'CompleteQuest', 'AddItem', 'TeleportTo', 'CancelQuestObjects'):
    assert forbidden not in production, forbidden
(out / 'Adventure.inc').write_text(production, encoding='utf-8')

# Exercise the actual newly added chat/sound routing branches. The ordinary
# range implementation remains byte-for-byte unchanged after these branches.
for filename, marker in [('CreatureTextMgrImpl.h', 'template<class Builder>\nvoid CreatureTextMgr::SendChatPacket'),
                         ('CreatureTextMgr.cpp', 'void CreatureTextMgr::SendNonChatPacket')]:
    text = (root / 'src/server/game/Texts' / filename).read_text(encoding='utf-8-sig')
    part = text[text.index(marker):]
    branch = part.index('    if (range == TEXT_RANGE_PERSONAL)')
    opening = part.index('{', branch)
    cursor, depth = opening + 1, 1
    while depth:
        depth += (part[cursor] == '{') - (part[cursor] == '}')
        cursor += 1
    name = 'ChatPersonal.inc' if filename.endswith('.h') else 'SoundPersonal.inc'
    (out / name).write_text(part[branch:cursor], encoding='utf-8')
