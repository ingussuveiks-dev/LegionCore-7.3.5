"""Actual retained callbacks/64-bit branches; client behavior is separate."""
from pathlib import Path
import runpy,json,struct
from capstone import CS_OP_MEM
p=runpy.run_path(str(Path(__file__).with_name('verify_sixth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
plan=json.loads(Path(__file__).with_name('SeventhTenRepairs.json').read_text())
for case in plan:
 if not case.get('cls'):continue
 rows=ins('?JustDied@'+case['cls']+'AI@');at,=sites(rows,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
 assert any(x.mnemonic=='je' and x.operands[0].imm>rows[at].address for x in rows[:at]),(case['key'],'null killer bypasses owner lookup')
 assert any(x.mnemonic=='test' and x.op_str=='rax, rax' and rows[i+1].mnemonic=='je' for i,x in enumerate(rows) if at<i<at+4),(case['key'],'missing owner guard')
 if case['key']=='Talbot':
  credit,=sites(rows,'?RewardPlayerAndGroupAtEvent@Player@@');assert any(x.mnemonic=='mov' and x.op_str.startswith('r8, qword ptr [') for x in rows[max(0,credit-7):credit]),'actual me reward source'
 if case['key']=='Netherweb':
  summon=sites(rows,'?SummonCreature@WorldObject@@')[0];credit,=sites(rows,'?KilledMonsterCredit@Player@@');assert rows[summon+1].mnemonic=='test' and rows[summon+2].mnemonic=='je' and summon<credit
 if case['key']=='Nerubar':
  cast=sites(rows,'?CastSpell@Unit@@')[0];credit,=sites(rows,'?KilledMonsterCredit@Player@@');assert rows[cast+1].mnemonic=='test' and rows[cast+2].mnemonic=='jne' and cast<credit
 print('PATH actual selected death callback',case['key'])
rows=ins('?Meets@Condition@@');selected=[]
for at in sites(rows,'?GetUInt64Value@Object@@'):
 if op(rows[max(0,at-3):at],'mov','edx, 0x52'):selected.append(rows[at:at+16])
assert len(selected)==2
assert op(selected[0],'cmp','r14, rcx') and op(selected[0],'cmp','r14, rax')
assert sum(op([x],'cmp','rax, rcx')for x in selected[1])==2
rows=ins('?LoadFromDB@Player@@');selected=[]
for at in sites(rows,'?GetUInt64@Field@@'):
 if op(rows[max(0,at-2):at],'lea','rcx, [r13 + 0x390]'):selected.append(rows[at:at+3])
assert len(selected)==1 and op(selected[0],'mov','rdi, rax') and op(selected[0],'mov','qword ptr [rsp + 0x100], rax')
assert b'Player (guidlow %llu) have problems with transport guid (%llu). Teleport to bind location.' in n['binary']
print('PASS actual Release seventh ten:8 owner/null-guard death callbacks,Talbot actual reward source,Netherweb real summon/Nerubar cast-OK before credit,Condition57 both64bit min/max branches,and saved transport64bit field/store/diagnostic. Prior67 paths retained;native/source/client scopes distinct.')
