"""Small independent x64 reader for the selected new implementation paths."""
from pathlib import Path
import bisect,functools,runpy
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM
import struct
n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
@functools.lru_cache(None)
def ins(prefix):
    address=n['symbol'](prefix);index=bisect.bisect_right(n['starts'],address-n['base'])-1
    begin,end,unwind=n['functions'][index]
    if not begin<=address-n['base']<end:
        end=min(a for a in n['symbols'].values()if a>address)-n['base']
        assert end+n['base']-address<=64,'Unknown leaf bounds: '+prefix
    else:
        origin=n['root'](begin,unwind)
        for b,e,u in n['functions'][index+1:]:
            if b!=end or n['root'](b,u)!=origin:break
            end=e
    at=n['offset'](address)
    rows=list(decoder.disasm(n['binary'][at:at+end+n['base']-address],address))
    # MSVC can include an RVA switch table after the final return within the
    # unwind range. Prove its image-base addressing and real in-body targets
    # before excluding those data bytes from instruction/call-site searches.
    for i,x in enumerate(rows):
        for operand in x.operands:
            if operand.type!=CS_OP_MEM or operand.mem.scale!=4 or not operand.mem.index:continue
            table=n['base']+operand.mem.disp
            if not address<table<end+n['base']:continue
            base_register=x.reg_name(operand.mem.base)
            proved=any(y.mnemonic=='lea'and y.op_str.startswith(base_register+', [rip')and y.address+y.size+y.operands[1].mem.disp==n['base']for y in rows[:i]if len(y.operands)>1 and y.operands[1].type==CS_OP_MEM)
            before=[y for y in rows if y.address<table]
            terminal=next((y for y in reversed(before)if y.mnemonic not in ('nop','int3')),None)
            # An epilogue may tail-call another independently bounded native
            # function. Such an unconditional jump cannot fall into the table.
            closed=terminal is not None and (terminal.mnemonic=='ret'or terminal.mnemonic=='jmp'and terminal.operands[0].type==CS_OP_IMM and (terminal.operands[0].imm<table or terminal.operands[0].imm-n['base'] in n['starts']))
            if not proved or not before or not closed or before[-1].address+before[-1].size!=table:continue
            raw=n['offset'](table);targets=[n['base']+struct.unpack_from('<I',n['binary'],raw+4*j)[0]for j in range(2)]
            assert all(address<=t<table and any(y.address==t for y in before)for t in targets),'Recognized switch table has invalid first two code targets'
            # MSVC's compressed switch maps a checked byte index to the RVA
            # table. Validate every remapped target, not just its first pair.
            remaps=[(j,y,o)for j,y in enumerate(rows[:i])for o in y.operands if y.mnemonic=='movzx'and o.type==CS_OP_MEM and o.size==1 and y.reg_name(o.mem.base)==base_register and o.mem.index and table<n['base']+o.mem.disp<end+n['base']]
            for j,y,o in remaps:
                remap=n['base']+o.mem.disp
                bounds=[z for z in rows[max(0,j-4):j]if z.mnemonic=='cmp'and z.operands[-1].type==CS_OP_IMM]
                assert len(bounds)==1,'Compressed switch requires a unique nearby index bound'
                bound=bounds[0];branch=rows[rows.index(bound)+1]
                assert branch.mnemonic=='ja'and branch.operands[0].type==CS_OP_IMM and address<=branch.operands[0].imm<table,'Compressed switch requires an in-code out-of-range branch'
                count=bound.operands[-1].imm+1
                assert 0<count<=end+n['base']-remap,'Compressed switch byte map exceeds unwind range'
                remap_at=n['offset'](remap);indices=n['binary'][remap_at:remap_at+count]
                assert table+4*(max(indices)+1)==remap,'Compressed switch RVA and byte tables must meet exactly'
                for entry in range(max(indices)+1):
                    target=n['base']+struct.unpack_from('<I',n['binary'],raw+4*entry)[0]
                    assert any(z.address==target for z in before),'Compressed switch target is not an instruction boundary'
            return before
    return rows

@functools.lru_cache(None)
def targets(prefix):return {n['resolve'](a)for name,a in n['symbols'].items()if name.startswith(prefix)}
def sites(rows,prefix):return [i for i,x in enumerate(rows)if x.mnemonic in ('call','jmp')and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)in targets(prefix)]
def values(rows):return {o.imm for x in rows for o in x.operands if o.type==CS_OP_IMM}
def callbacks(cls,method):return [name for name in n['symbols']if name.startswith('?_Do_call@')and cls+'@'in name and method+'@'in name]
operators={n['resolve'](a):name for name,a in n['symbols'].items()if name.startswith('??R<lambda')or(name.startswith('??$invoke@')and'<lambda_'in name)}
def expanded(name):
    rows=ins(name);result=list(rows)
    for x in rows:
        if x.mnemonic not in ('call','jmp')or not x.op_str.startswith('0x'):continue
        operator=operators.get(n['resolve'](int(x.op_str,16)))
        if operator:result+=ins(operator)
    return result
