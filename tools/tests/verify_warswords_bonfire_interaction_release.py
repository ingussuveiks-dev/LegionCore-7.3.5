"""Require the new actual Use slot, native player cast and completed-hit guard."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from bonfire_interaction_scope import verify
import verify_five_ten_20261113_release
verify()
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_bonfire_release.py')))
n,ins,sites=p['n'],p['ins'],p['sites']
scope='@go_mystic_bonfireAI@go_mystic_bonfire@@'
use=ins('?GossipUse'+scope);hello=ins('?GossipHello'+scope);eligible=ins('?Eligible'+scope)
def pairs(rows):return {(x.mnemonic,x.op_str)for x in rows}
def reaches(rows,start):
    by={x.address:i for i,x in enumerate(rows)};pending=[start];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address not in by:continue
        seen.add(address);x=rows[by[address]]
        if x.mnemonic=='ret':continue
        if x.mnemonic.startswith('j'):
            assert x.operands[0].type==CS_OP_IMM,'Unexpected indirect control flow'
            pending.append(x.operands[0].imm)
            if x.mnemonic=='jmp':continue
        pending.append(x.address+x.size)
    return seen
admit,=sites(use,'?Eligible'+scope);pending,=sites(use,'?IsNonMeleeSpellCast@Unit@@');cast,=sites(use,'?CastSpell@Unit@@')
assert admit<pending<cast
assert ('xor','r8d, r8d')in pairs(use[max(0,admit-4):admit]),'Use must select the stricter start admission'
assert {('mov','r8d, 0x30771'),('xor','r9d, r9d')}<=pairs(use[cast-12:cast]),'Use must request actual198513 without triggered cast flags'
for point,branch_name in ((admit,'je'),(pending,'jne')):
    branch=use[point+2]
    assert use[point+1].mnemonic=='test'and branch.mnemonic==branch_name
    assert use[cast].address not in reaches(use,branch.operands[0].imm),'Denied/busy use can still cast'
assert {('movzx','ebx, r8b'),('mov','r8d, ebx'),('xor','r8d, 1'),('or','r8d, 0x10')}<=pairs(eligible),'IN_USE blocks starting another cast;completed native hit still checks NOT_SELECTABLE'
for forbidden in ('?UpdateAchievementCriteria@Player@@','?SetGoState@GameObject@@','?AddItem@Player@@','?CompleteQuest@Player@@','?TeleportTo@Player@@'):
    assert not sites(use,forbidden),'Use must not grant completion/presentation: '+forbidden
credit,=sites(hello,'?UpdateAchievementCriteria@Player@@');light,=sites(hello,'?SetGoState@GameObject@@')
step=next(i for i in sites(hello,'?GetCurrentStep@Scenario@@')if i>credit)
assert credit<step<light and ('xor','edx, edx')in pairs(hello[light-3:light])
ack=hello[step+2]
assert hello[step+1].mnemonic=='cmp'and hello[step+1].op_str=='eax, 1'and ack.mnemonic=='jne'
assert hello[light].address not in reaches(hello,ack.operands[0].imm),'Unacknowledged progress can light the object'
factory=ins('?GetAI@go_mystic_bonfire@@');tables=[]
for x in factory:
    for operand in x.operands:
        if x.mnemonic=='lea'and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
            at=n['offset'](x.address+x.size+operand.mem.disp)
            if struct.unpack_from('<Q',n['binary'],at+0x48)[0]==n['symbol']('?GossipHello'+scope):tables.append(at)
at,=tables
assert struct.unpack_from('<Q',n['binary'],at+0x40)[0]==n['symbol']('?GossipUse'+scope),'Actual bonfire factory vtable does not install the new Use hook'
observe='?Observe'+scope
assert sites(use,observe)[0]<cast and sites(hello,observe)[0]<credit,'Bound Use/Hello must retain the live-context observation'
for marker in (b'Warswords bonfire native Use:',b'Warswords bonfire open-lock hit:',b'Warswords bonfire cast request:',b'Warswords bonfire cast prepare result='):
    assert marker in n['binary'],'Deployed linked code lacks the scoped diagnostic: '+str(marker)
print('PASS linked actual bonfire Use/Hello slots,scoped stricter start/nontriggered198513/busy-cast rejection,no Use credit,completed-hit admission and actual ACK before fire/fog;retained full Release checks returned normally')
