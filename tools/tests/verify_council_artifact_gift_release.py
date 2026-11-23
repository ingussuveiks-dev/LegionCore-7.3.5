"""Verify the linked ritual jump, native circle action and scene handoff."""
from pathlib import Path
import runpy

from capstone import CS_OP_IMM


pe = runpy.run_path(str(Path(__file__).with_name("verify_smart_linked_cast_retry_release.py")),
                  init_globals={"PE_HELPERS_ONLY": True})


def calls(caller, *targets):
    addresses = pe["calls"](caller)
    for target in targets:
        matching = [name for name in pe["symbols"] if name.startswith(target)]
        assert matching and any(pe["symbol"](name) in addresses for name in matching), (caller, target)


jump = "?EffectJumpDest@Spell@@"
motion = "?MoveJump@MotionMaster@@QEAAXMMMMMMI_N"
restore = "?MaintainRitual@CouncilArtifactGift@@"

for method, expected in (
    (jump, {234670, 234679}),
    (motion, {234670, 234679, 1220}),
    (restore, {234667, 13567, 90418, 377268}),
):
    values = {operand.imm for instruction in pe["instructions"](method)
              for operand in instruction.operands if operand.type == CS_OP_IMM}
    assert expected <= values, (method, expected - values)

calls(jump, "?GetSpellTargetPosition@SpellMgr@@", "?CalculateJumpSpeeds@Spell@@", motion)
calls(restore, "?IsUnitAffected@AreaTrigger@@", "?IsInArea@AreaTrigger@@", "?AffectUnit@AreaTrigger@@")
calls("?Update@EventObject@@", restore, "?IsEOMeetConditions@ScriptMgr@@", "?OnEventObject@ScriptMgr@@")
calls("?EffectTeleUnitsFaceCaster@Spell@@", "?BeginTransfer@CouncilArtifactGift@@")
calls("?HandleMoveTeleportAck@WorldSession@@", "?OnArrival@CouncilArtifactGift@@")
print("PASS linked Council landing, field membership and native action, ritual transfer and near-ACK scene handoff")
