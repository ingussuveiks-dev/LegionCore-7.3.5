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





scope='npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@';method='?DamageTakenAfterScaling@'+scope
body=instructions(method);targets=native['calls'](method)
for prefix in ('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?GetHealth@Unit@@QEBA_KPEAV1@@','?GetScenario@ScenarioMgr@@','?GetCurrentStep@Scenario@@'):
 assert native['symbol'](prefix) in targets, f'Missing actual capture guard {prefix}'
values={op.imm for ins in body for op in ins.operands if op.type==CS_OP_IMM}
assert {41924,1607,1078,1}<=values
for prefix in ('?UpdateAchievementCriteria@Player@@','?CastSpell@Unit@@','?TeleportTo@Player@@','?DespawnOrUnsummon@Creature@@'):
 matches={native['resolve'](a) for n,a in native['symbols'].items() if n.startswith(prefix)}
 assert not matches & targets, f'Damage guard must not force capture effects: {prefix}'
selected=[]
for ins in instructions('?GetAI@npc_akaari_shadowgore@@'):
 for op in ins.operands:
  if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
   slots=struct.unpack_from('<80Q',native['binary'],native['offset'](ins.address+ins.size+op.mem.disp))
   if slots[0xa8//8]==native['symbol'](method):selected.append(slots)
assert len(selected)==1 and selected[0][0xa0//8]==native['symbol']('?DamageTaken@UnitAI@@')
owner=native['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@');health=native['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@')
sites=lambda target:[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==target]
owner_site,=sites(owner);health_site,=sites(health);assert owner_site<health_site
assert body[health_site-1].op_str=='rdx, rbp' and any(x.op_str=='rbp, rdx' for x in body[:owner_site]), 'Relative health lost incoming attacker'
assert body[health_site+2].op_str=='rcx, rax' and body[health_site+3].mnemonic=='jb'
assert body[health_site+4].op_str=='rax, 1' and body[health_site+5].mnemonic=='jbe'
assert body[health_site+6].op_str=='eax' and body[health_site+6].mnemonic=='dec'
assert body[health_site+5].operands[0].imm==body[health_site+8].address and body[health_site+8].op_str=='eax, eax'
assert body[health_site+7].mnemonic=='jmp' and body[health_site+7].operands[0].imm==body[health_site+9].address
assert body[health_site+9].op_str=='dword ptr [r14], eax'
assert body[health_site+3].operands[0].imm>body[health_site+9].address
print('PASS selected opening Akaari capture postA8/raw inheritedA0, native1078/0/41924/owner/relative-health guard without forced effects. Native host proves final-damage clamp; event-driven capture is separately regressed; client traversal remains pending.')
