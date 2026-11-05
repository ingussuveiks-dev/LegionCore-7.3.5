"""Verify native nearest-player entry, range/life state and last-searcher route."""
from pathlib import Path
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM
from sharasdal_erunak_entry_pe import sites
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_smoke_offer_release.py')))
n, original = p['n'], p['ins']
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def code(name):
    try:
        return original(name)
    except AssertionError as error:
        if 'Missing runtime bounds' not in str(error):
            raise
        at = n['symbol'](name)
        end = min(n['resolve'](x) for x in n['symbols'].values() if n['resolve'](x) > at)
        assert end - at <= 128, 'Unbounded native leaf'
        start = n['offset'](at)
        return list(decoder.disasm(n['binary'][start:start + end - at], at))


pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}
entry = code('?FindNearestPlayer@WorldObject@@')
ctor_name = '??0NearestPlayerInObjectRangeCheck@Trinity@@'
predicate_name = '??RNearestPlayerInObjectRangeCheck@Trinity@@'
construct, = sites(n, entry, ctor_name)
assert {('movzx', 'r9d, r8b'), ('movaps', 'xmm2, xmm1'), ('movaps', 'xmm7, xmm1')} <= pairs(entry[:construct])
assert not sites(n, entry, '?GetVisibilityRange@WorldObject@@')
# The linker folds equal-layout Any/Nearest constructors. The concrete
# last-searcher route below distinguishes the actual shrinking predicate.
ctor = code(ctor_name)
assert {('movss', 'dword ptr [rcx + 8], xmm2'), ('mov', 'qword ptr [rcx], rdx'),
        ('mov', 'byte ptr [rcx + 0xc], r9b')} <= pairs(ctor)
predicate = code(predicate_name)
distance, = sites(n, predicate, '?IsWithinDistInMap@WorldObject@@')
metric, = sites(n, predicate, '?GetDistance@WorldObject@@')
assert distance < metric
assert {('cmp', 'byte ptr [rcx + 0xc], 0'), ('cmp', 'dword ptr [rdx + 0xa18], 0'),
        ('movss', 'xmm2, dword ptr [rcx + 8]'), ('movss', 'dword ptr [rbx + 8], xmm0')} <= pairs(predicate)
for at in [i for i, x in enumerate(predicate) if (x.mnemonic, x.op_str) in
           {('cmp', 'dword ptr [rdx + 0xa18], 0'), ('test', 'al, al')}]:
    assert predicate[at + 1].mnemonic in ('jne', 'je')
    assert predicate[metric].address not in reachable(predicate, predicate[at + 1].operands[0].imm)
# Follow actual Cell/Map template calls to the concrete shrinking predicate;
# finding an unreferenced PlayerLastSearcher symbol is not sufficient.
family = {n['resolve'](at): name for name, at in n['symbols'].items()
          if 'PlayerLastSearcher@VNearestPlayerInObjectRangeCheck@Trinity@@' in name}


def targets(body):
    return {n['resolve'](x.operands[0].imm) for x in body
            if x.mnemonic in ('call', 'jmp') and x.operands[0].type == CS_OP_IMM}


pending = list(targets(entry) & family.keys())
assert pending and ('movss', 'dword ptr [rsp + 0x20], xmm7') in pairs(entry)
seen = set()
while pending:
    at = pending.pop()
    if at in seen:
        continue
    seen.add(at)
    calls = targets(code(family[at]))
    if n['symbol'](predicate_name) in calls:
        break
    pending.extend(calls & family.keys() - seen)
else:
    raise AssertionError('Native last-searcher route does not reach the repaired predicate')
creature = code('?SelectNearestPlayer@Creature@@')
legacy, = sites(n, creature, ctor_name)
assert ('mov', 'r9b, 1') in pairs(creature[:legacy])
print('PASS native WorldObject.FindNearestPlayer: incoming radius/alive -> concrete nearest predicate, actual Cell/Map PlayerLastSearcher route, native map/distance test and shrinking metric, optional life guard; existing Creature selector explicitly live-only. All affected callers/client gameplay remain separate.')
