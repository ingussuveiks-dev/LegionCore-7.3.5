"""Check the actual linked Fragment branch and both encounter cleanup callers."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name("verify_smart_linked_cast_retry_release.py")))
ins = n["instructions"]
fragment = ins("?SpellHitTarget@boss_harbaron@@")
assert {194325, 98761, 23, 8, 2, 4, 5000} <= values(fragment)
# The actual two difficulty comparisons share the four-fragment assignment;
# the other branch retains two. The selected register controls the summon loop.
start, = [i for i, x in enumerate(fragment) if x.mnemonic == "cmp" and x.op_str == "al, 0x17"]
window = fragment[start:start + 10]
assert [(x.mnemonic, x.op_str) for x in window[4:8]] == [
    ("cmp", "al, 8"), ("mov", "bpl, 2"),
    ("jne", window[6].op_str), ("mov", "bpl, 4")]
assert window[1].mnemonic == "je" and window[1].operands[0].imm == window[7].address
assert window[6].operands[0].imm == window[8].address
assert any(x.mnemonic == "cmp" and x.op_str == "dil, bpl" for x in fragment)
cleanup = ins("?DespawnSouls@boss_harbaron@@")
remove, = sites(n, cleanup, "?RemoveSoulFragmentAuras@@")
despawn, = sites(n, cleanup, "?DespawnOrUnsummon@Creature@@")
assert remove < despawn
for caller in ("?EnterEvadeMode@boss_harbaron@@", "?JustDied@boss_harbaron@@"):
    assert sites(n, ins(caller), "?DespawnSouls@boss_harbaron@@")
assert sites(n, ins("?JustDied@npc_harbaron_soul_fragment@@"), "?RemoveSoulFragmentAuras@@")
aura = ins("?RemoveSoulFragmentAuras@@")
assert {194344, 194327} <= values(aura) and 198551 not in values(aura)
for method in ("?GetAnyOwner@Unit@@", "?GetAura@Unit@@", "?HasAura@Unit@@", "?RemoveAurasDueToSpell@Unit@@"):
    assert sites(n, aura, method), method
assert sites(n, ins("?AddLegionScripts@@"), "?AddSC_boss_harbaron@@")
print("PASS actual linked Harbaron: Mythic23/Keystone8 select4, other modes2; exact-owner/caster aura cleanup before despawn from evade/death and fragment death; native fixed stun untouched; registered Legion loader. Client combat remains separate.")
