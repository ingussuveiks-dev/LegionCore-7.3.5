"""Check the deployed trigger-model selection, including its unwind-free tail jumps."""
from pathlib import Path
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_departure_release.py')))
native = previous['native']
address = native['symbol']('?ChooseDisplayId@ObjectMgr@@')
following = min(value for value in native['symbols'].values() if value > address)
assert 0 < following - address <= 128, 'Unexpected leaf selection bounds'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
start = native['offset'](address)
instructions = list(decoder.disasm(native['binary'][start:start + following - address], address))
assert any(instruction.mnemonic == 'test' and any(operand.type == CS_OP_IMM and operand.imm == 128
           for operand in instruction.operands) for instruction in instructions), 'Missing trigger flag gate'
targets = {native['resolve'](instruction.operands[0].imm) for instruction in instructions
           if instruction.mnemonic == 'jmp' and instruction.operands[0].type == CS_OP_IMM}
assert native['symbol']('?GetFirstInvisibleModel@CreatureTemplate@@') in targets
assert native['symbol']('?GetRandomValidModelId@CreatureTemplate@@') in targets
assert native['symbol']('?GetCreatureModelInfo@ObjectMgr@@') in native['calls']('?GetFirstInvisibleModel@CreatureTemplate@@')
print('PASS: actual Release trigger flag128 selects the native invisible carrier/state visual, retaining the ordinary random-model path')
