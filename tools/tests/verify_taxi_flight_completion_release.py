"""Verify the selected taxi finalizer cleans only a truly completed native route."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
method = '?DoFinalize@FlightPathMovementGenerator@@'
body = instructions(method)


def sites(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


clear = next(index for index, entry in enumerate(body) if entry.mnemonic == 'and' and entry.op_str == 'dword ptr [rdx + 0x3140], 0xfffffeff')
cleanup, = sites('?CleanupAfterTaxiFlight@Player@@')
dismount, = sites('?Dismount@Unit@@')
legacy = body[dismount - 1].address
assert clear < cleanup < dismount
for operands, branch in (('rax, rdx', 'je'), ('rax, rdx', 'jb'), ('qword ptr [rcx + 0x68], 0', 'jne'), ('qword ptr [rax + 0x20], 1', 'jne')):
    assert any(entry.mnemonic == 'cmp' and entry.op_str == operands and body[index + 1].mnemonic == branch
               and body[index + 1].operands[0].imm == legacy for index, entry in enumerate(body[:cleanup]))
assert body[cleanup - 2].op_str == 'dl, 1', 'Completed cleanup must retain native zone update'
assert native['symbol']('?ClearTaxiDestinations@PlayerTaxi@@') in native['calls']('?CleanupAfterTaxiFlight@Player@@')
cast, = sites('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@')
assert cleanup < cast < dismount
assert any(entry.op_str == 'r8d, 0x9af' for entry in body[cleanup:cast])
assert any(entry.op_str == 'byte ptr [rbx + 0x3d18], 0' for entry in body[cleanup:cast])
assert all(any(entry.op_str == operand for entry in body[cleanup:cast]) for operand in ('rdx, rbx', 'rcx, rbx', 'r9b, 1'))
assert len(sites('?SetFallInformation@Player@@')) == 2
assert sites('?empty@PlayerTaxi@@')[0] > dismount
assert sites('?setOnlineOfflineState@HostileRefManager@@QEAAX_N@Z')[0] > dismount
assert sites('?ResummonPetTemporaryUnSummonedIfAny@Player@@')[0] > dismount
assert native['symbol']('?SummonLastSummonedBattlePet@Player@@') in native['calls'](method)
wrapper = '?Finalize@?$MovementGeneratorMedium@VPlayer@@VFlightPathMovementGenerator@@@@'
assert native['symbol'](wrapper) == native['symbol'](method) or native['symbol'](method) in native['calls'](wrapper)
tables = []
for entry in instructions('?MoveTaxiFlight@MotionMaster@@'):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<8Q', native['binary'], native['offset'](address))
        try:
            if native['resolve'](slots[2]) == native['symbol'](wrapper):
                tables.append(address)
        except ValueError:
            pass
assert len(tables) == 1, 'Native MoveTaxiFlight factory does not select repaired Finalize slot0x10'
print('PASS supplied Release MoveTaxiFlight factory/Finalize10 reaches repaired DoFinalize: IN_FLIGHT cleared before native cleanup; nonempty path/actual final point/no pending segment/exactly one terminal taxi node gate cleanup(true), fall and original hostile2479. Legacy interruptions, benchmark and both pet helpers retained. No raw user taxi state or real client/restart/portrait proof.')
