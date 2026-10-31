"""Verify existing native spell/click/linked consumers used by ascent data."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites,fields
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions'];evaluate='?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@'
check=ins('?CheckCast@Spell@@');lookup,=sites(n,check,'?GetConditionsForNotGroupedEntry@ConditionMgr@@')
assert any(x.op_str=='r8d, 0x11' for x in check[lookup-6:lookup])
assert any(x.op_str=='r9d, dword ptr [r9]' for x in check[lookup-6:lookup])
checks=sites(n,check,evaluate);assert len(checks)==1 and checks[0]>lookup
at=checks[0];assert check[at+1].op_str=='al, al' and check[at+2].mnemonic=='jne'
prepare=ins('?prepare@Spell@@');admission,=sites(n,prepare,'?CheckCast@Spell@@')
casts=sites(n,prepare,'?cast@Spell@@');assert casts and all(admission<at for at in casts)
assert prepare[admission-2].op_str=='dl, 1'
assert prepare[admission+3].op_str=='eax, eax' and prepare[admission+4].mnemonic=='je'
for caller in ('?canSeeSpellClickOn@Player@@','?HandleSpellClick@Unit@@'):
 body=ins(caller);assert sites(n,body,'?GetConditionsForSpellClickEvent@ConditionMgr@@') and sites(n,body,evaluate)
assert sites(n,ins('?BuildValuesUpdate@Unit@@'),'?canSeeSpellClickOn@Player@@')
click=ins('?HandleSpellClick@Unit@@');cast,=sites(n,click,'?CastSpell@Unit@@')
assert any(x.op_str=='r9d, r9d' and x.mnemonic=='xor' for x in click[cast-6:cast])
aura=ins('?HandleAuraSpecificMods@Aura@@');lookups=sites(n,aura,'?GetSpellLinked@SpellMgr@@')
remove=[at for at in lookups if any(x.mnemonic=='neg' and x.op_str=='r8d' for x in aura[at-4:at])
        and any(x.mnemonic=='xor' and x.op_str=='edx, edx' for x in aura[at-4:at])]
removed,=remove;next_lookup=next(at for at in lookups if at>removed)
default_casts=[at for at in sites(n,aura,'?CastSpell@Unit@@') if removed<at<next_lookup]
assert default_casts and {0x34,0x38}<=fields(aura[removed:next_lookup]) # native removeMask/actiontype
default=default_casts[0]
assert any(x.op_str=='r9b, 1' for x in aura[default-9:default])
assert any(x.op_str=='r8d, dword ptr [rsi]' for x in aura[default-9:default])
print('PASS selected canonical Spell.prepare strict CheckCast before native cast calls; actual spell-ID/source17 caster+target evaluation, source18 visible/dynamic-flag/click lookups and ordinary click cast; negative-aura link type0/negated ID reaches native triggered cast with row effect and removal/action fields. SQL/64 native context+mode tests are separate; no full spell-engine/client/automatic-ascent claim.')
