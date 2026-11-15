"""Actual linked callback receivers, including conditional tail calls."""
from pathlib import Path
import re,hashlib
from prepare_core_delayed_owners import verify
verify()
reader=Path(__file__).with_name('verify_pending_ai_release.py');s=reader.read_text();c={'__file__':str(reader)};exec(compile(s[:s.index('plan=json.loads')],str(reader),'exec'),c)
n=c['n'];ins=c['instructions'];expanded=c['expanded']
def sites(rows,prefix):
    targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
    return [i for i,x in enumerate(rows) if (x.mnemonic=='call' or x.mnemonic.startswith('j')) and x.op_str.startswith('0x') and n['resolve'](int(x.op_str,16)) in targets]
def receiver(rows,at):
    state={'rcx':'closure'}
    for x in rows[:at]:
        if x.mnemonic=='mov':
            m=re.fullmatch(r'(r\w+), qword ptr \[(r\w+)(?: \+ (0x[0-9a-f]+|\d+))?\]',x.op_str)
            if m:
                state[m[1]]='owner' if state.get(m[2])=='closure' and int(m[3] or '0',0)==8 else 'indirect';continue
            m=re.fullmatch(r'(r\w+), (r\w+)',x.op_str)
            if m:state[m[1]]=state.get(m[2],'other');continue
        if x.mnemonic=='lea':
            m=re.fullmatch(r'(r\w+), \[(r\w+) \+ (0x[0-9a-f]+|\d+)\]',x.op_str)
            if m:state[m[1]]='embedded' if state.get(m[2])=='owner' else 'other';continue
        if x.mnemonic=='call':
            for reg in ('rax','rcx','rdx','r8','r9','r10','r11'):state.pop(reg,None)
        elif x.mnemonic in ('lea','xor','mov','movzx'):state[x.op_str.split(',')[0]]='other'
    return state.get('rcx')
for method,payload,expected in [
    ('SummonCreatureDelay@ScriptedAI','?SummonCreature@WorldObject@@','owner'),
    ('_EnterCombat@BossAI','?RepopPlayersAtGraveyard@InstanceScript@@','owner'),
    ('InitializeAI@CritterAI','?MoveRandom@MotionMaster@@','embedded'),
    ('ProcessAction@SmartScript','?SetReactState@Creature@@','owner')]:
    names=[k for k in n['symbols'] if k.startswith('?_Do_call@') and method+'@@' in k and k.endswith('EEAAXXZ')]
    selected=[]
    for name in names:
        rows=expanded(name);matches=sites(rows,payload)
        if matches:selected.append((rows,matches))
    assert len(selected)==1,(method,'missing/ambiguous real payload closure')
    rows,matches=selected[0];at,=matches
    assert receiver(rows,at)==expected,(method,'payload still follows retired source AI/script')
    if expected=='embedded':
        assert any(x.mnemonic=='mov' and x.op_str=='ecx, 0x1e' for x in rows[:at])
        assert any(x.mnemonic=='mov' and x.op_str=='edx, 0x32' for x in rows[:at])
        assert any(x.mnemonic=='cmp' and x.op_str=='al, 2' for x in rows[:at]),'waypoint exclusion changed'
        assert any(x.mnemonic=='cmp' and x.op_str=='al, 3' for x in rows[:at]),'motion-type bound changed'
    print('CASE linked core',method,'direct captured owner before real payload PASS')
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp[1],16)==n['u32'](n['pe']+8)
print('PASS4 linked core delayed owners; original3 full source files exactly pinned,real summon/state/motion/repop endpoints,no retired AI/script receiver,matching PE-map;SHA256='+hashlib.sha256(n['binary']).hexdigest().upper())
