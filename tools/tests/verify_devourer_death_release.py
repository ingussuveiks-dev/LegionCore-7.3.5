"""Check the linked final Vigfus death callback, owner bridge and selected vtable."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp.group(1), 16) == struct.unpack_from('<I', native['binary'], native['pe'] + 8)[0], 'Canonical PE/map link timestamps differ'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, unwind = native['functions'][index]
    assert begin <= address - native['base'] < end, f'Missing runtime bounds: {prefix}'
    origin = native['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native['functions'][index + 1:]:
        if tail_begin != end or native['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    start = native['offset'](address)
    return list(decoder.disasm(native['binary'][start:start + native['base'] + end - address], address))





for scope, factory, event, conversation in (
 ('npc_devourer_zirusAI@npc_devourer_zirus@@','?GetAI@npc_devourer_zirus@@',50298,1890),
 ('npc_akaari_shadowgore_lastAI@npc_akaari_shadowgore_last@@','?GetAI@npc_akaari_shadowgore_last@@',50300,1947)):
 method='?JustDied@'+scope;body=instructions(method);targets=native['calls'](method)
 for prefix in ('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?UpdateAchievementCriteria@Player@@','?CreateConversation@Conversation@@'):
  assert native['symbol'](prefix) in targets, f'Missing selected death dependency {scope}: {prefix}'
 values={op.imm for ins in body for op in ins.operands if op.type==CS_OP_IMM}
 assert {92,event,conversation} <= values
 if event==50298:assert {248983,248984,248910,249028,4} <= values, 'Original Zirus door payload missing'
 tables=[]
 for ins in instructions(factory):
  for op in ins.operands:
   if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
    at=native['offset'](ins.address+ins.size+op.mem.disp)
    if struct.unpack_from('<Q',native['binary'],at+0x130)[0]==native['symbol'](method):tables.append(at)
 assert len(tables)==1, 'Factory misses repaired death130'
 owner=native['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@');credit=native['symbol']('?UpdateAchievementCriteria@Player@@')
 owner_site=next(i for i,ins in enumerate(body) if ins.mnemonic=='call' and ins.operands[0].type==CS_OP_IMM and native['resolve'](ins.operands[0].imm)==owner)
 credit_site=next(i for i,ins in enumerate(body) if ins.mnemonic=='call' and ins.operands[0].type==CS_OP_IMM and native['resolve'](ins.operands[0].imm)==credit)
 assert owner_site<credit_site
 assert any(ins.mnemonic=='test' and ins.op_str=='rdx, rdx' and body[i+1].mnemonic=='je' and body[i+1].operands[0].imm>body[owner_site].address for i,ins in enumerate(body[:owner_site])), 'Null killer not guarded'
 owner_register=body[owner_site+1].op_str.split(',')[0]
 assert body[owner_site+1].mnemonic=='mov' and body[owner_site+1].op_str.endswith(', rax')
 assert any(ins.mnemonic=='test' and ins.op_str in (f'{owner_register}, {owner_register}', 'rax, rax') and body[i+1].mnemonic=='je' and body[i+1].operands[0].imm>body[credit_site].address for i,ins in enumerate(body[:credit_site])), 'Unresolved owner reaches credit'
 if event==50298:
  assert any(ins.mnemonic=='mov' and ins.op_str=='rbp, qword ptr [r14 + 0x20]' for ins in body[:credit_site]), 'Missing actor fallback for conversation/door cleanup'
assert any(ins.mnemonic=='call' and ins.operands[0].type==CS_OP_MEM and ins.operands[0].mem.disp==0x130 for ins in instructions('?Kill@Unit@@'))
print('PASS selected Zirus/Akaari death130 callbacks use native owner and original50298/50300/conversations; original door payload and native death dispatcher retained. Native host covers eligibility lookup/reentrant repeated Kill and failure cleanup; client sequence pending.')
