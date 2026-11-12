from linked_player_abi import inherited_operand
"""Bind the instance-owned corridor cursor to the actual selected Release."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from release_scalar_getter import decode,evaluate


def check(n,ins):
    hr='@npc_hrutnirAI@npc_hrutnir@@'
    shared='@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'
    def sites(body,prefix):
        targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type==CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]
    def find(body,text,mnemonic=None):
        return next(i for i,o in enumerate(body) if o.op_str==text and (mnemonic is None or o.mnemonic==mnemonic))
    def has(body,*texts):
        for text in texts: find(body,text)
    remember=ins('?RememberMagnarRoute'+shared)
    query=find(remember,'qword ptr [rax + 0x50]','call')
    store=find(remember,'dword ptr [rsi + 0x388], r15d','mov')
    has(remember[:query],'edx, 0x42b','r12d, dword ptr [rsi + 0x35c]',
        'byte ptr [rsi + 0x356], 0','byte ptr [rsi + 0x357], 0','eax, 0x38d','eax, 3',
        'eax, 0x178d4',inherited_operand('qword ptr [rbx + 0x3a88], 0x5835a'),'qword ptr [rbx + 8], rax',
        'byte ptr [rbx + 0x858], 0','r14, qword ptr [rbx + 0x9c0]','dword ptr [rcx + 0x1c], 0x1b57')
    # A caller provides only the generation. The cursor is pulled through the
    # actual AI vtable, bounded and identity/epoch revalidated before storage.
    assert len(sites(remember,'?GetCreature@Map@@'))==2
    assert len(sites(remember,'?GetScenario@ScenarioMgr@@'))==2
    has(remember[query:store],'ecx, [r15 - 1]','ecx, 0xc','r12d, dword ptr [rsi + 0x35c]',
        'qword ptr [rdi + 0x9c0], r14','byte ptr [rsi + 0x356], 0','byte ptr [rsi + 0x357], 0',
        'eax, 0x178d4',inherited_operand('qword ptr [rdi + 0x3a88], 0x5835a'),'qword ptr [rdi + 8], rax',
        'byte ptr [rdi + 0x858], 0','dword ptr [rcx + 0x1c], 0x1b57','eax, 0x38d','eax, 3')
    assert sites(remember[query:store],'??8ObjectGuid@@')
    setter=ins('?SetData'+shared)
    call,=sites(setter,'?RememberMagnarRoute'+shared)
    has(setter[:call],'edx, 0x42b','edx, r8d')
    branch=find(setter,'edx, 0x42b','cmp')+1
    assert setter[branch].mnemonic=='jne' and setter[branch].operands[0].imm>setter[call].address
    reset=ins('?Initialize'+shared)
    has(reset,'dword ptr [rdi + 0x388], ebp')
    creation=ins('?OnCreatureCreate'+shared)
    clear=find(creation,'dword ptr [rdi + 0x388], 0','mov')
    has(creation[:clear],'eax, 0x178d4',inherited_operand('qword ptr [rbx + 0x3a88], 0x5835a'),'rcx, [rdi + 0x336]')
    assert creation[clear-1].mnemonic=='je' and creation[clear-1].operands[0].imm>creation[clear].address
    assert sites(creation[:clear],'??9ObjectGuid@@')
    has(creation[clear:],'xmmword ptr [rdi + 0x336], xmm0')
    leaf=decode(n,'?GetData'+shared)
    for credit in (0,1):
        for delivered in (0,1):
            for cursor in (0,1,4,13):
                assert evaluate(leaf,1067,{0x356:credit,0x357:delivered,0x388:cursor})==(cursor if credit and delivered else 0)
    factory=ins('?GetInstanceScript@instance_shields_rest@@');tables=[]
    for o in factory:
        for operand in o.operands:
            if o.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                at=n['offset'](o.address+o.size+operand.mem.disp)
                if struct.unpack_from('<Q',n['binary'],at+0xb8)[0]==n['symbol']('?SetData'+shared):tables.append(at)
    assert len(tables)==1
    assert struct.unpack_from('<Q',n['binary'],tables[0]+0xb0)[0]==n['symbol']('?GetData'+shared)
    getter=ins('?GetData'+hr)
    has(getter,'edx, 0x42b','eax, dword ptr [rcx + 0x18c]','eax, 0xc','r8d, 0x429','dword ptr [rcx + 0x1c], 0x1b57')
    assert sites(getter,'?IsMagnarOfferContext'+hr) and sites(getter,'?HasMagnarChamberRequest'+hr)
    restore=ins('?RestoreMagnarChamber'+hr)
    has(restore,'edx, 0x42b','ecx, [rbx - 1]','ecx, 0xc','dword ptr [rdi + 0x18c], ebx')
    route=ins('?UpdateMagnarChamberRoute'+hr)
    move,=sites(route,'?MovePoint@MotionMaster@@')
    record=find(route,'edx, 0x42b','mov');callback=next(i for i in range(record,len(route)) if route[i].mnemonic=='call')
    assert callback<move
    has(route[callback:move],'qword ptr [rsi + 0x9c0], rdi','ebx, dword ptr [rdi + 0xa8]','r14d, dword ptr [rdi + 0x18c]')
    assert route[callback+1].op_str=='qword ptr [rsi + 0x9c0], rdi' and route[callback+2].mnemonic=='jne'
    assert route[callback+2].operands[0].imm>route[move].address
    for section in (remember,setter[:call+1],restore,route):
        for forbidden in ('?NearTeleportTo@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@'):
            assert not sites(section,forbidden)
    print('PASS selected cursor: real original companion current AI pulls1..13 through actual instance GetData/SetData vtable; guard/delivery/909-3/phase/fullGUID/epoch double validation,Initialize/newGUID clear,metadata-only restore and entrance restart; native motion revalidates after receipt callback. Real arrivals/normal paths/13s/6s and client evidence certified separately.')


if __name__=='__main__':
    p=runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    check(p['native'],p['instructions'])
