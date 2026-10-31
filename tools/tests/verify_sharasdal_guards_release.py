"""Verify the selected native Tidecaller death and shared criterion producer."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_rescue_release.py')))
n=retained['n']; instructions=n['instructions']; sites=retained['sites']; values=retained['values']
suffix='@instance_sharasdal_InstanceMapScript@instance_sharasdal@@'
factory=instructions('?GetInstanceScript@instance_sharasdal@@')
table_pattern=re.compile(r'\s+\w+:\w+\s+\?\?_7instance_sharasdal_InstanceMapScript@instance_sharasdal@@6B@\s+([0-9a-fA-F]{16})')
tables=[int(m[1],16) for line in (n['args'].release/'worldserver.map').read_text(errors='replace').splitlines() if (m:=table_pattern.match(line))]
table,=tables
assert table in retained['references'](factory)
slots=struct.unpack_from('<128Q',n['binary'],n['offset'](table))
selected={}
for method in ('CreatureDies','OnPlayerEnter','Update'):
    # Vtables retain actual issued tail wrappers. resolve() follows a leading
    # jump, so Update's public symbol resolves to its UpdateGuard tail target.
    addresses={address for name,address in n['symbols'].items() if name.startswith('?'+method+suffix)}
    indexes=[i for i,value in enumerate(slots) if value in addresses]
    slot,=indexes
    selected[method]=slot*8
assert n['symbol']('?Update'+suffix)==n['symbol']('?UpdateGuard'+suffix) or sites(instructions('?Update'+suffix),'?UpdateGuard'+suffix)
kill=instructions('?Kill@Unit@@')
dispatch=[]
for i,x in enumerate(kill):
    if x.mnemonic=='mov' and x.operands[0].type==CS_OP_REG and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==selected['CreatureDies']:
        register=x.operands[0].reg
        if any(y.mnemonic=='call' and y.operands[0].type==CS_OP_REG and y.operands[0].reg==register for y in kill[i+1:i+6]):
            dispatch.append(i)
dispatch_site,=dispatch
assert any(x.mnemonic=='mov' and x.op_str=='r8, r13' for x in kill[dispatch_site+1:dispatch_site+5])
assert any(x.mnemonic=='mov' and x.op_str=='rdx, r15' for x in kill[dispatch_site+1:dispatch_site+5])
death=instructions('?CreatureDies'+suffix)
assert {102831,362076,1066,40341}<=values(death)
assert sites(death,'?Receipt@SharasdalGuard@@') and sites(death,'?Recipient@SharasdalGuard@@')
assert sites(death,'?AddQuestDelayedEvent@Player@@') and sites(death,'?UpdateGuard'+suffix)
recipient=instructions('?Recipient@SharasdalGuard@@')
assert {102826,362063,105298,362066}<=values(recipient)
assert sites(recipient,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
assert sites(recipient,'?IsInSameGroupWith@Player@@') and sites(recipient,'?Eligible@SharasdalGuard@@')
eligible=instructions('?Eligible@SharasdalGuard@@')
assert 40341 in values(eligible) and sites(eligible,'?GetQuestStatus@Player@@') and sites(eligible,'?GetQuestRewardStatus@Player@@')
receipt=instructions('?Receipt@SharasdalGuard@@')
assert 47197 in values(receipt)
update=instructions('?UpdateGuard'+suffix)
assert {40341,1066,1000,30}<=values(update)
credit_sites=sites(update,'?UpdateAchievementCriteria@Player@@')
credit_site,=credit_sites
window=update[max(0,credit_site-10):credit_site]
assert any(x.mnemonic=='mov' and x.op_str=='r8d, 0xc325' for x in window)
assert any(x.mnemonic=='mov' and x.op_str=='edx, 0x5c' for x in window)
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rdi], 1' for x in update[:credit_site])
assert any(i>credit_site for i in sites(update,'?Receipt@SharasdalGuard@@'))
# The selected instance stores a full GUID and compares the original quest's
# shared token, without retaining a defeated actor or Unit-owned delayed task.
assert any(x.mnemonic=='cmp' and x.op_str.startswith('qword ptr [r8 + 0x18],') for x in update)
assert any(x.mnemonic=='cmp' and x.op_str=='dword ptr [rsi + 0x320], 0x1e' for x in update)
enter=instructions('?OnPlayerEnter'+suffix)
assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x31a and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==0 for x in enter)
movement=instructions('?MovementInform@npc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@')
assert not sites(movement,'?UpdateAchievementCriteria@Player@@') and 49957 not in values(movement)
for body in (death,update,recipient):
    for prefix in ('?AddItem@Player@@','?CompleteQuest@Player@@','?AddDelayedEvent@Unit@@'):
        assert not sites(body,prefix)
print('PASS selected native instance factory/vtable and Unit::Kill dispatch, exact102831/362076 death, owner/pet/guardian and known-ally native-group recipient, full40341 quest token, actual47197 acknowledgement, sole92/49957 call under busy guard, stage gates/finite30 retries/re-entry. Erunak waypoint has no achievement call. Native shared scenario, other actors and Breaking Bonds are retained; actual client combat/re-entry remains separate.')
print({'guard_vtable_byte_offsets':selected})
