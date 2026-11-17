from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1])
source = (root / 'src/server/scripts/Legion/ClassHalls/tirisfal_wards.cpp').read_text(encoding='utf-8-sig')
part = source[source.index('namespace TirisfalWards'):source.index('\nclass npc_tirisfal_holy_ward')]
for forbidden in ('KilledMonsterCredit', 'UpdateAchievementCriteria', 'AddItem', 'TeleportTo', 'CompleteQuest', 'SetHealth'):
    assert forbidden not in part, forbidden
(out / 'Ward.inc').write_text(part, encoding='utf-8')

# Execute the actual native destination-selection prefix used by target106.
# The later random-offset branch is outside this spell's DEFAULT/NONE path.
source = (root / 'src/server/game/Spells/Spell.cpp').read_text(encoding='utf-8-sig')
start = source.index('void Spell::SelectImplicitDestDestTargets(')
end = source.index('    if (targetType.GetDirectionType() == TARGET_DIR_NONE)', start)
(out / 'Destination.inc').write_text(source[start:end] + '\nassert(false && "Unsupported destination test path");\n}\n', encoding='utf-8')
