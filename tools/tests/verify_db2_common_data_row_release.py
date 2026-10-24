"""Verify forward record offsets in all canonical regular common-data decoders."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
decoders = [name for name in native['symbols'] if name.startswith('??$RecordGetVarInt@') and '@DB2FileLoaderRegularImpl@@' in name]
assert len(decoders) == 6, 'Missing scalar regular decoder instantiations'
for name in decoders:
    body = instructions(name)
    divisions = [index for index, entry in enumerate(body) if entry.mnemonic == 'idiv']
    assert len(divisions) == 1, f'Common row calculation missing or ambiguous: {name}'
    index = divisions[0]
    start, difference = body[index - 3:index - 1]
    assert start.mnemonic == 'mov' and start.operands[0].type == CS_OP_REG and start.reg_name(start.operands[0].reg) == 'rax' and start.operands[1].type == CS_OP_REG, 'Dividend does not begin with the actual record pointer'
    assert difference.mnemonic == 'sub' and difference.operands[1].type == CS_OP_MEM and difference.operands[1].mem.disp == 0x20, 'Common row subtracts record from base, not base from record'
    assert body[index - 4].op_str == 'ecx, dword ptr [rcx + 0xc]' and body[index].op_str == 'rcx', 'Common row no longer divides by native RecordSize'
    record = start.reg_name(start.operands[1].reg)
    assert body[index + 1].op_str == f'rdx, {record}' and body[index + 3].op_str == 'r8, rax', 'Actual record and forward quotient do not reach the ID resolver'
    consumer = body[index + 4]
    assert consumer.mnemonic == 'call' and consumer.operands[0].type == CS_OP_IMM and native['resolve'](consumer.operands[0].imm) == native['symbol']('?RecordGetId@DB2FileLoaderRegularImpl@@'), 'Common-data lookup bypasses native ID resolution'
assert native['symbol']('?AutoProduceData@DB2FileLoader@@') in native['calls']('?Load@DB2StorageBase@@'), 'Canonical storage no longer uses native file decoding'
assert native['calls']('?AutoProduceData@DB2FileLoaderRegularImpl@@') & {native['symbol'](name) for name in decoders}, 'Regular table expansion does not invoke verified scalar decoding'
print('PASS: canonical six regular scalar instantiations subtract base from actual record, divide by RecordSize and pass forward row plus record to native ID resolution; actual storage/regular expansion consumers retained. Malformed files, other compression defects and rendered client visuals are not certified.')
