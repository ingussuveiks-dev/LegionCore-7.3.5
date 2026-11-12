from linked_player_abi import inherited_operand
"""Registered Akaari reset retires only its own native mirror images."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from sharasdal_erunak_entry_pe import sites

p=runpy.run_path(str(Path(__file__).with_name('verify_akaari_opening_release.py')))
n,ins,method=p['n'],p['ins'],p['method']
pairs=lambda rows:{(x.mnemonic,x.op_str) for x in rows}
reset,body=[ins(method(x)) for x in ('Reset','ResetClones')]
retire,=sites(n,reset,method('ResetClones'))
assert retire<sites(n,reset,'?Reset@EventMap@@')[0]<sites(n,reset,'?GetPlayer@ObjectAccessor@@')[0]
# Native map lookup copies the original complete GUID list before any callback.
assert {('mov','rdx, qword ptr [rax + 0x848]'),('cmp','dword ptr [rax + 0x20], 0x19c89'),('cmp','dword ptr [rdi + 0x20], 0x19c89'),('movups','xmm0, xmmword ptr [rbx + 0x10]'),('movups','xmmword ptr [rax + 0x10], xmm0')} <= pairs(body)
lookup,=sites(n,body,'?GetCreature@ObjectAccessor@@')
identities=sites(n,body,'??8ObjectGuid@@')
assert len(identities)==2 and lookup<identities[0]<identities[1]
# First comparison is returned GUID versus the copied list GUID. The second
# compares the summon's full128-bit summoner field with this actual source.
assert {('lea','rdx, [rbp - 0x20]'),('movups',inherited_operand('xmm0, xmmword ptr [rdi + 0x3b64]')),('movups','xmmword ptr [rbp - 0x30], xmm0'),('lea','rcx, [rbp - 0x30]'),('cmp','eax, 0x19c89')} <= pairs(body)
virtuals=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM]
assert len(virtuals)==1
unsummon=virtuals[0]
assert body[unsummon].op_str=='qword ptr [rax + 0x2b8]'
assert pairs(body[unsummon-3:unsummon])=={('mov','rax, qword ptr [rdi]'),('xor','edx, edx'),('mov','rcx, rdi')}
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
table,=[int(x,16) for x in re.findall(r'\s+\w+:\w+\s+\?\?_7TempSummon@@6BUnit@@@\s+([0-9a-fA-F]{16})',text)]
target,=struct.unpack_from('<Q',n['binary'],n['offset'](table)+0x2b8)
assert n['resolve'](target)==n['symbol']('?UnSummon@TempSummon@@')
for index in identities:
 assert body[index+1].op_str=='al, al' and body[index+2].mnemonic=='je'
 # A denial advances to the next GUID, without visiting this iteration's call.
 assert body[index+2].operands[0].imm==body[unsummon+1].address
# Native producers209233..235 become an unsigned three-value range check.
assert {('mov','edx, 0x7c'),('add','eax, 0xfffcceaf'),('cmp','eax, 2')} <= pairs(body)
gate,=[i for i,x in enumerate(body) if (x.mnemonic,x.op_str)==('cmp','eax, 2')]
assert body[gate+1].mnemonic=='ja' and body[gate+1].operands[0].imm==body[unsummon+1].address
assert ('test','byte ptr [rax + 0x30e8], 1') in pairs(body) # actual summon mask
for forbidden in ('?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@','?FindNearestCreature@WorldObject@@','?DespawnCreaturesInArea@WorldObject@@'):
 assert not sites(n,body,forbidden)
print('PASS registered first-Akaari Reset: native105609 owner-index GUID snapshot, actual TempSummon/full128 identity+summoner+entry and209233..235 producer guards, zero-delay native UnSummon before original event/owner reset. No nearest-area retirement/progression/AI replacement. Native counterfactual and repaired lifecycle cover reaggro/retry/foreign controls; actual client route remains separate.')
