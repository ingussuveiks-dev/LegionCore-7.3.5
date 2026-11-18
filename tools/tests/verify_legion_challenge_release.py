"""Inspect actual final-linked lifecycle, completion, persistence and shared hook."""
from pathlib import Path
import re
import runpy
from sharasdal_erunak_entry_pe import sites, values

n=runpy.run_path(str(Path(__file__).with_name("verify_smart_linked_cast_retry_release.py")))
checks=[
 ("?Update@Challenge@@","?CanStart@Challenge@@"),
 ("?Update@Challenge@@","?Start@Challenge@@"),
 ("?Start@Challenge@@","?CanStart@Challenge@@"),
 ("?Reward@Scenario@@","?Complete@Challenge@@"),
 ("?Complete@Challenge@@","?HitTimer@Challenge@@"),
 ("?Complete@Challenge@@","?SaveChallengeToDB@ChallengeMgr@@"),
]
for caller,target in checks:
 assert sites(n,n["instructions"](caller),target),(caller,target)
assert 5000 in values(n["instructions"]("?OnPlayerDiesForScript@Challenge@@")),"Missing native five-second running death penalty"
save=n["instructions"]("?SaveChallengeToDB@ChallengeMgr@@")
assert any(any(x.mnemonic=="mov" and x.op_str=="dl, 9" for x in save[max(0,i-4):i])
           for i in sites(n,save,"?setUInt16@PreparedStatementBase@@")),"Missing actual tenth route parameter"
source=(Path(__file__).resolve().parents[2]/"src/server/database/Database/Implementation/CharacterDatabase.cpp").read_text(encoding="utf-8-sig")
sql=re.search(r'PrepareStatement\(CHAR_INS_CHALLENGE, "([^"]+)"',source)[1]
assert sql.count("?")==10 and "`ChallengeID`" in sql and sql.encode() in n["binary"]
glare=n["instructions"]("?Register@spell_coen_blinding_glare_SpellScript@spell_coen_blinding_glare@@")
assert {255,15}<=values(glare),"Missing actual EFFECT_ALL/enemy-area hook"
print("PASS final-linked native lifecycle/start/death/completion calls,Scenario reward dispatch,actual10-column prepared save/tenth route parameter andshared glare EFFECT_ALL/enemy-area registration. Native regressions verify behavior;client remains separate.")
