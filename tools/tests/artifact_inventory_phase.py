"""Extract the production phase gates and artifact inventory notifications."""
from pathlib import Path
import re, sys
ROOT = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1])
def read(p): return (ROOT / p).read_text(encoding='utf-8-sig')
def block(s, start):
    a = s.index(start); b = s.index('{', a); depth = 1; i = b + 1
    while depth:
        depth += (s[i] == '{') - (s[i] == '}'); i += 1
    return s[a:i]
header = read('src/server/game/Conditions/ConditionMgr.h')
constants = re.findall(r'^\s*(CONDITION_\w+)\s*=\s*(\d+)', header, re.M)
(out/'Enums.inc').write_text('enum ConditionTypes {' + ','.join(f'{k}={v}' for k,v in constants) + '};', encoding='utf-8')
phase = read('src/server/game/Maps/PhaseMgr.cpp')
(out/'Supported.inc').write_text(block(phase, 'bool PhaseMgr::IsConditionTypeSupported('), encoding='utf-8')
source = read('src/server/game/Conditions/ConditionMgr.cpp')
cases = ''
for start, end in [('CONDITION_ITEM_EQUIPPED','CONDITION_ZONEID'), ('CONDITION_QUESTREWARDED','CONDITION_QUEST_NONE'), ('CONDITION_SPEC_ID','CONDITION_ON_TRANSPORT')]:
    a = source.index('        case ' + start + ':'); b = source.index('        case ' + end + ':', a)
    cases += source[a:b]
(out/'Conditions.inc').write_text(cases, encoding='utf-8')
a = source.index('    if (NegativeCondition)'); b = source.index('\n}', a)
(out/'ConditionResult.inc').write_text(source[a:b], encoding='utf-8')
(out/'Inventory.inc').write_text(block(read('src/server/game/Entities/Item/ArtifactAcquisitionInventory.h'), 'namespace ArtifactAcquisitionInventory'), encoding='utf-8')
player = read('src/server/game/Entities/Player/Player.cpp')
for name in ('ItemAddedQuestCheck', 'ItemRemovedQuestCheck'):
    body = block(player, 'void Player::' + name + '(')
    prefix = body[:body.index('    for (uint8 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)')]
    assert 'GetArtifactID()' in prefix and 'NotifyConditionChanged(update)' in prefix
    (out/(name+'.inc')).write_text(prefix+'}\n', encoding='utf-8')
# These native call sites drive bank/storage, equipment and specialization changes.
assert 'StoreItem' in player[player.index('void Player::SwapItem('):player.index('void Player::ItemAddedQuestCheck(')]
assert 'phaseUdateData.AddConditionType(CONDITION_SPEC_ID)' in player
assert 'CONDITION_ITEM_EQUIPPED' in block(player, 'void Player::_ApplyItemMods(')
print('PASS extracted actual supported phase types, native condition cases/result, inventory helper and pre-early-return notifications')
