"""Certify same-instance delivered-chamber adoption in the selected Release."""
from pathlib import Path
import runpy,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(n,ins):
    hr='@npc_hrutnirAI@npc_hrutnir@@'
    shared='@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'
    def sites(body,prefix):
        targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i,op in enumerate(body) if op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_IMM and n['resolve'](op.operands[0].imm) in targets]
    def has(body,*operands):
        for operand in operands:assert any(op.op_str==operand for op in body),'Missing installed chamber operand: '+operand
    helper=ins('?RestoreMagnarChamber'+hr)
    context,=sites(helper,'?IsMagnarOfferContext'+hr)
    for operand in ('byte ptr [rcx + 0xa5], 0','byte ptr [rcx + 0x88], 0','byte ptr [rcx + 0x89], 0',
                    'byte ptr [rcx + 0x5d], 0','qword ptr [rcx + 0x60], 0','qword ptr [rcx + 0x78], 0','dword ptr [rcx + 0xa8], 0'):
        index=next(i for i,op in enumerate(helper[:context]) if op.op_str==operand)
        branch=index+1
        if helper[branch].mnemonic=='mov':
            assert helper[branch].op_str=='rdi, rcx'
            branch+=1
        assert helper[branch].mnemonic=='jne' and helper[branch].operands[0].imm>helper[context].address
    has(helper,'edx, 0x42a','edx, 0x423','edx, 0x424','r8d, 0x429',
        'dword ptr [rdi + 0xa8], eax','byte ptr [rdi + 0xa5], 1','byte ptr [rdi + 0x58], 1')
    assert sites(helper,'?GetInstanceScript@WorldObject@@') and sites(helper,'??9ObjectGuid@@')
    calls=[(i,op) for i,op in enumerate(helper) if op.mnemonic=='call' and op.operands[0].type!=CS_OP_IMM]
    assert len(calls)==4
    # Two actual read-only receipt/guard virtual calls, original companion GUID,
    # then the current epoch immediately before the three metadata stores.
    for (index,_),value in zip(calls[:2],('edx, 0x42a','edx, 0x423')):
        has(helper[max(0,index-6):index],value)
        assert helper[index+1].op_str=='eax, eax' and helper[index+2].mnemonic=='je'
        assert helper[index+2].operands[0].imm>helper[calls[-1][0]].address
    has(helper[calls[-1][0]-4:calls[-1][0]],'edx, 0x424')
    for forbidden in ('?MovePoint@MotionMaster@@','?Talk@CreatureAI@@','?AddDelayedEvent@FunctionProcessor@@',
                      '?UpdateAchievementCriteria@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?NearTeleportTo@',
                      '?SceneCompleted@Player@@','?CreateConversation@','?DoAction'):
        assert not sites(helper,forbidden),'Metadata restoration bypasses native flow'
    update=ins('?UpdateAI'+hr);los=ins('?MoveInLineOfSight'+hr)
    assert sites(update,'?RestoreMagnarChamber'+hr)[0]<sites(update,'?UpdateMagnarReoffer'+hr)[0]<sites(update,'?UpdateVictim@CreatureAI@@')[0]
    restore,=sites(los,'?RestoreMagnarChamber'+hr)
    has(los[:restore],'byte ptr [rdx + 0x16], 4')
    assert los[restore+1].op_str=='byte ptr [rbx + 0x58], 0' and los[restore+2].mnemonic=='jne'
    assert restore<sites(los,'?Talk@CreatureAI@@')[0]<sites(los,'?AddDelayedEvent@FunctionProcessor@@')[0]
    # Select and decode the actual shared GetData leaf, rather than invent a
    # persisted flag or relying on source text for the readonly receipt.
    address=n['symbol']('?GetData'+shared);following=min(a for a in n['symbols'].values() if a>address)
    assert 0<following-address<=96
    decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
    leaf=list(decoder.disasm(n['binary'][n['offset'](address):n['offset'](address)+following-address],address))
    assert [(op.mnemonic,op.op_str) for op in leaf[:9:2]]==[('sub','edx, 6'),('sub','edx, 0x41d'),('sub','edx, 1'),('sub','edx, 3'),('cmp','edx, 3')]
    assert 6+0x41d+1+3+3==1066
    assert leaf[9].mnemonic=='jne'
    false=leaf[9].operands[0].imm
    assert [(op.mnemonic,op.op_str) for op in leaf[10:16:2]]==[('cmp','byte ptr [rcx + 0x356], 0'),('cmp','byte ptr [rcx + 0x357], 0'),('mov','eax, 1')]
    assert leaf[11].mnemonic==leaf[13].mnemonic=='je' and leaf[11].operands[0].imm==leaf[13].operands[0].imm==false
    assert leaf[15].mnemonic=='ret'
    zero=next(i for i,op in enumerate(leaf) if op.address==false)
    assert leaf[zero].mnemonic=='xor' and leaf[zero].op_str=='eax, eax' and leaf[zero+1].mnemonic=='ret'
    factory=ins('?GetInstanceScript@instance_shields_rest@@');tables=[]
    for op in factory:
        for operand in op.operands:
            if op.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                offset=n['offset'](op.address+op.size+operand.mem.disp)
                if struct.unpack_from('<Q',n['binary'],offset+0xb0)[0]==address:tables.append(offset)
    assert len(tables)==1,'Factory does not select actual chamber receipt'
    print('PASS selected fresh Hruthnir chamber: current909-3/actor/local-claim/exact original GUID,real instance1066=guard AND delivered plus1059/current1060 metadata-only adoption; idle and Player LOS callers precede reoffer/old intro. Original13s/6s/497/native1062 clocks remain independently certified. No scene/motion/progression grant; native replacement and client/cold route evidence separate.')


if __name__=='__main__':
    p=runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    check(p['native'],p['instructions'])
