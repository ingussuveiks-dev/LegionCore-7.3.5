"""Prove both selected wound consumers use the real final-heal notification."""
from pathlib import Path
import runpy
import struct

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_brutes_release.py')))
bridge=runpy.run_path(str(Path(__file__).with_name('verify_creature_heal_applied_release.py')))
n=retained['n'];instructions=n['instructions']
tables=retained['guard']['retained']['grash']['old']['previous']['selected_tables']
for role,pending in [('npc_erunak_stonespeaker',0x5b),('npc_adel_waterspeaker',0x5a)]:
    suffix='@'+role+'AI@'+role+'@@'
    table=tables[role]
    for method,slot in [('OnHealApplied',0x288),('UpdateLifecycle',0x110),('Reset',0x28)]:
        assert struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?'+method+suffix)
    assert struct.unpack_from('<Q',n['binary'],table+0xb0)[0]==n['symbol']('?HealReceived@UnitAI@@')
    body=instructions('?OnHealApplied'+suffix)
    while body and body[-1].mnemonic=='int3':body.pop()  # leaf padding up to next public symbol
    assert [(x.mnemonic,x.op_str) for x in body]==[
        ('cmp','byte ptr [rcx + 0x59], 0'),
        ('je',body[1].op_str),
        ('mov',f'byte ptr [rcx + {pending:#x}], 1'),
        ('ret','')]
    assert body[1].operands[0].imm==body[3].address
    # No actor/health/quest credit/role mutation from the notification itself.
    life=instructions('?UpdateLifecycle'+suffix)
    if role=='npc_erunak_stonespeaker':
        assert [(x.mnemonic,x.op_str) for x in life[:3]]==[
            ('lea','r8, [rcx + 0x59]'),('lea','rdx, [rcx + 0x5b]'),('mov','rcx, qword ptr [rcx + 0x20]')]
        assert life[3].mnemonic=='jmp' and n['resolve'](life[3].operands[0].imm)==n['symbol']('?Recover@SharasdalCompanion@@')
    else:
        assert retained['guard']['retained']['sites'](life,'?Recover@SharasdalCompanion@@')
    reset=instructions('?Reset'+suffix)
    if role=='npc_erunak_stonespeaker':
        assert reset[0].mnemonic=='mov' and reset[0].op_str==f'byte ptr [rcx + {pending:#x}], 0'
    else:
        assert any(x.mnemonic=='mov' and x.op_str==f'byte ptr [rbx + {pending:#x}], 0' for x in reset)
print('PASS both actual selected Erunak/Adelee applied0x288 hooks only mark waiting wounds; pre-heal0xb0 is native empty, unchanged40pct lifecycle/reset and rescue retained. Actual DealHeal/ModifyHealth/final-positive Notify current+suspended locked dispatch verified; no client/full-chain claim')
