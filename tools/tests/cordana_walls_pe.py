"""Check the real horizontal-wall counter/bound/X spawn argument in x64 code."""
import re,struct

def verify(native,instructions):
    names=[name for name in native['symbols']if name.startswith('??R<lambda')and'boss_cordanaAI'in name and'SummonCreepings'in name]
    assert len(names)==1,'Expected the single real Cordana wall callback'
    rows=instructions(names[0])
    horizontal,=[i for i,x in enumerate(rows)if x.mnemonic=='comiss'and x.op_str.endswith(', dword ptr [rcx + 0x30]')]
    vertical,=[i for i,x in enumerate(rows)if x.mnemonic=='comiss'and x.op_str.endswith(', dword ptr [rcx + 0x34]')]
    assert vertical<horizontal,'Original vertical and horizontal branches missing'
    counter=rows[horizontal].op_str.split(',')[0]
    tail=rows[horizontal+1:]
    advance=next(x for x in tail if x.mnemonic=='addss')
    assert advance.op_str.startswith(counter+', '),'Horizontal condition must compare the advancing X coordinate'
    increment=advance.op_str.split(', ')[1]
    load=[x for x in rows[:horizontal]if x.mnemonic=='movss'and x.op_str.startswith(increment+', dword ptr [rip + ')]
    assert load,'Missing horizontal counter increment'
    source=load[-1];displacement=int(re.search(r'\[rip \+ (0x[0-9a-f]+)\]',source.op_str)[1],0)
    assert struct.unpack_from('<f',native['binary'],native['offset'](source.address+source.size+displacement))[0]==-2.5,'Authored spacing changed'
    body=tail[:tail.index(advance)]
    assert any(x.mnemonic=='movaps'and x.op_str=='xmm2, '+counter for x in body),'Advancing X must reach the actual native X spawn argument'
    assert any(x.mnemonic=='movss'and x.op_str==counter+', dword ptr [rax]'for x in rows[vertical:horizontal]),'Horizontal counter must load native start X'
    spawn_targets={native['resolve'](address)for name,address in native['symbols'].items()if name.startswith('?SummonCreature@WorldObject@@')}
    assert any(x.mnemonic=='call'and x.op_str.startswith('0x')and native['resolve'](int(x.op_str,16))in spawn_targets for x in body),'Original native creature spawn call missing'
    assert any(x.mnemonic=='mov'and x.op_str=='edx, 0x187f0'for x in body),'Original100336entry changed'
    assert any(x.mnemonic=='mov'and x.op_str=='dword ptr [rsp + 0x38], 0x4650'for x in body),'Original18000ms lifetime changed'
    return names[0]
