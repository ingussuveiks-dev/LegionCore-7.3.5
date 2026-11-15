"""Real kill-credit closure must dispatch before the optional corpse lookup."""
from pathlib import Path
import re,hashlib
from prepare_delayed_kill_credit import verify
verify();reader=Path(__file__).with_name('verify_pending_ai_release.py');s=reader.read_text();c={'__file__':str(reader)};exec(compile(s[:s.index('plan=json.loads')],str(reader),'exec'),c);n=c['n']
name,=[k for k in n['symbols'] if k.startswith('?_Do_call@') and '_RewardKillCredit@KillRewarder@@' in k and k.endswith('EEAAXXZ')];rows=c['expanded'](name)
def site(prefix):
    targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
    hits=[i for i,x in enumerate(rows) if x.mnemonic=='call' and x.op_str.startswith('0x') and n['resolve'](int(x.op_str,16)) in targets]
    at,=hits;return at
template=site('?GetCreatureTemplate@ObjectMgr@@');credit=site('?KilledMonster@Player@@');lookup=site('?GetCreature@ObjectAccessor@@')
assert template<credit<lookup,'corpse lookup still gates native confirmed death credit'
assert rows[template-2].op_str=='edx, dword ptr [rbx + 8]','entry not copied into pending request'
assert rows[template+1].mnemonic=='test' and rows[template+1].op_str=='rax, rax'
assert rows[template+2].mnemonic=='je' and int(rows[template+2].op_str,16)==rows[credit+1].address,'missing template no longer skips only its credit'
assert rows[credit-2].op_str=='rdx, rax','native template result not passed to ordinary dispatcher'
assert rows[credit-1].op_str=='rcx, qword ptr [rbx + 0x10]','player anchor changed'
assert rows[lookup+2].mnemonic=='test' and rows[lookup+2].op_str=='rax, rax' and rows[lookup+3].mnemonic=='je','live reward-player marking lacks optional corpse guard'
assert any(x.mnemonic=='movups' and x.op_str=='xmm0, xmmword ptr [rbx + 0x18]' for x in rows[template:credit]),'original victim GUID not delivered'
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'));assert stamp and int(stamp[1],16)==n['u32'](n['pe']+8)
print('PASS linked death-credit closure:copied native entry/template and original GUID to eligible Player::KilledMonster before optional corpse lookup;live reward marking guard retained,full file pinned;SHA256='+hashlib.sha256(n['binary']).hexdigest().upper())
