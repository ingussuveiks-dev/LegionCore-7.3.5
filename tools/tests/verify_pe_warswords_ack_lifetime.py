"""All16 real weak-lifetime refusal edges must remain present in the PE."""
from pathlib import Path
import ast
from five_ten_20261111_pe import ins, n
from warswords_ack_lifetime_pe import guards
from capstone import CS_OP_IMM

source = Path(__file__).with_name('verify_warswords_final_intro_release.py')
node, = [x for x in ast.parse(source.read_text()).body if isinstance(x, ast.FunctionDef) and x.name == 'reachable']
scope = {'CS_OP_IMM': CS_OP_IMM}
exec(compile(ast.Module(body=[node], type_ignores=[]), str(source), 'exec'), scope)
actor = 'npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
outer, = [k for k in n['symbols'] if k.startswith('??R<lambda_1>@?1??MoveInLineOfSight@' + actor)]
inner, = [k for k in n['symbols'] if k.startswith('??R<lambda_1>@?1??UpdateBlessing@' + actor)]
p = dict(native=n, instructions=ins, reachable=scope['reachable'], actor=actor, outer=outer, callback=inner)
edges, sites = guards(p)
cast, = sites(ins(outer), '?CastSpell@Unit@@')
zero, = [x for x in ins(outer)[:cast] if x.mnemonic == 'xor' and x.op_str == 'ebx, ebx']
mutations = list(edges) + [(outer, zero.address)]
original = n['binary']
for prefix, address in mutations:
    branch, = [x for x in ins(prefix) if x.address == address]
    try:
        damaged = bytearray(original)
        offset = n['offset'](address)
        damaged[offset:offset+branch.size] = b'\x90' * branch.size
        n['binary'] = bytes(damaged)
        ins.cache_clear()
        try:
            guards(p)
        except (AssertionError, StopIteration):
            pass
        else:
            raise AssertionError('Missing weak-lifetime refusal edge accepted')
    finally:
        n['binary'] = original
        ins.cache_clear()
assert len(guards(p)[0]) == 16 and n['binary'] == original
print('PASS four actual queued-channel weak guards/all16 refusal edges plus preserved zero;17 removed guard/zero instructions rejected;original PE unchanged')
