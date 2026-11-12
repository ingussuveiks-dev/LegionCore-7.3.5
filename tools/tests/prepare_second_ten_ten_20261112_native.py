from pathlib import Path
import sys
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);require_fixed()
assert any(r['key']=='PetJournalRemovalCleanup'for r in PLAN['changes'])
source=(ROOT/'src/server/game/Handlers/BattlePetHandler.cpp').read_text(encoding='utf-8-sig')
(out/'SecondJournalRemovalNative.inc').write_text(block(source,r'static void RemoveBattlePetJournalEntry\(')+'\n')
print('Prepared exact complete journal-removal helper; native player/companion/roster APIs remain actual linked implementations.')
