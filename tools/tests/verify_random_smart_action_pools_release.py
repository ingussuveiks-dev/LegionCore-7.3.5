"""Follow real empty-pool branches, compacted stores and bounded native RNG."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP, X86_REG_RSP

p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n=p['native'];body=p['instructions']('?ProcessAction@SmartScript@@')
indices={x.address:i for i,x in enumerate(body)}


def check(marker):
    references=[]
    for i,x in enumerate(body):
        for o in x.operands:
            if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
                try:
                    at=n['offset'](x.address+x.size+o.mem.disp)
                    if marker in n['binary'][at:at+240].split(b'\0')[0]:references.append(i)
                except ValueError:pass
    assert len(references)==1, ('Missing actual dispatcher diagnostic',marker)
    ref=references[0]
    guards=[i for i in range(max(0,ref-100),ref)
            if body[i].mnemonic=='test' and all(o.type==CS_OP_REG for o in body[i].operands)
            and body[i].operands[0].reg==body[i].operands[1].reg
            and body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm>body[ref].address]
    assert len(guards)==1, ('Missing nonempty guard',marker)
    guard=guards[0];count=body[guard].reg_name(body[guard].operands[0].reg)
    assert any(x.mnemonic=='inc' and x.op_str==count for x in body[guard-20:guard]), 'Guard must test populated count'
    start=indices[body[guard+1].operands[0].imm]
    randoms=[i for i in range(start,min(start+40,len(body)))
             if body[i].mnemonic=='call' and body[i].operands[0].type==CS_OP_IMM
             and n['resolve'](body[i].operands[0].imm)==n['symbol']('?urand@@')]
    assert randoms, ('No native RNG on nonempty branch',marker)
    draw=randoms[0]
    assert body[draw-1].mnemonic=='xor' and body[draw-1].op_str=='ecx, ecx', 'Native lower bound must remain zero'
    upper=body[draw-2]
    if upper.mnemonic=='dec':
        assert count=='edx' and upper.op_str=='edx', 'Native maximum must decrement populated count'
    else:
        assert upper.mnemonic=='lea' and upper.operands[0].type==CS_OP_REG and upper.reg_name(upper.operands[0].reg)=='edx'
        assert upper.operands[1].type==CS_OP_MEM and upper.operands[1].mem.disp==-1
        assert upper.reg_name(upper.operands[1].mem.base)=={'esi':'rsi','ebx':'rbx','edi':'rdi'}[count], 'Maximum not populated count minus one'
    load=body[draw+2]
    assert body[draw+1].op_str=='eax, eax' and load.mnemonic=='mov' and load.operands[1].type==CS_OP_MEM
    pool=load.operands[1].mem
    assert pool.base==X86_REG_RSP and pool.scale==4 and load.reg_name(pool.index)=='rax'
    assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.base==pool.base
               and x.operands[0].mem.disp==pool.disp and x.operands[0].mem.scale==4
               for x in body[max(0,guard-25):guard]), 'Selected array differs from populated compact pool'
    assert any(x.mnemonic=='jmp' and not body[start].address<=x.operands[0].imm<=body[draw+2].address
               for x in body[ref:start]), 'Empty diagnostic branch must exit selection'
    return draw,load


emote,_=check(b'has no nonzero emote entries.')
phase,load=check(b'has no nonzero event phase entries.')
assert n['symbol']('?HandleEmoteCommand@Unit@@') in {
    n['resolve'](x.operands[0].imm) for x in body[emote+1:emote+12]
    if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM}, 'Selected emote does not reach native Unit command'
# SetPhase is fully inlined and removed from this link. IncPhase still has an
# actual symbol; its positive path reads and writes the mEventPhase field.
increment=n['symbol']('?IncPhase@SmartScript@@')
offset=n['offset'](increment)
increment_body=[]
for instruction in p['decoder'].disasm(n['binary'][offset:offset+64],increment):
    increment_body.append(instruction)
    if instruction.mnemonic=='ret':break
members={x.operands[0].mem.disp for x in increment_body
         if x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].size==4}
assert len(members)==1, 'Native IncPhase has no unique phase member write'
member=members.pop()
assert any(x.mnemonic=='mov' and x.operands[1].type==CS_OP_MEM
           and x.operands[1].mem.disp==member for x in increment_body), 'IncPhase member write has no matching phase read'
assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==member
           and x.operands[1].type==CS_OP_REG and x.operands[1].reg==load.operands[0].reg
           for x in body[phase+3:phase+8]), 'Selected phase is not written to the actual native phase member'
print('PASS actual SmartScript dispatcher emote/phase compacted-count guards and diagnostic exits, count-1 native inclusive RNG and indexed reads from populated pools; real per-unit emote consumer and native phase member. Full extracted ASan384-case probes separately cover empty cleanup/multiple targets/weighting; no client visual/quest traversal claim.')
