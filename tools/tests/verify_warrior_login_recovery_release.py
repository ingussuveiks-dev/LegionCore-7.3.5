from linked_player_abi import inherited_operand
"""Verify the actual pre-transport Warrior recovery and late quest admission."""
import runpy
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warrior_scenario_resume_release.py')))
n, ins = previous['n'], previous['ins']

if any(k.startswith('?Find@AcquisitionScenarioRoute@@') for k in n['symbols']):
    runpy.run_path(str(Path(__file__).with_name('verify_acquisition_login_recovery_release.py')), init_globals={'previous': previous})
else:

    def sites(body, prefix):
        targets = {n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
        return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type == CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]

    def find(body, text, start=0, mnemonic=None):
        return next(i for i,o in enumerate(body[start:],start) if o.op_str == text and (mnemonic is None or o.mnemonic == mnemonic))

    def rip(o):
        return next(o.address+o.size+a.mem.disp for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)

    # This leaf has no stack frame and legitimately no Windows unwind record.
    # Use its exact next public-symbol boundary, never extend a surrounding frame.
    a=n['symbol']('?Find@WarriorScenarioRoute@@');z=min(v for v in n['symbols'].values() if v>a)
    decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
    leaf=list(decoder.disasm(n['binary'][n['offset'](a):n['offset'](z)],a))
    assert [o.mnemonic for o in leaf] == ['lea','lea','nop','cmp','je','add','cmp','jne','xor','ret']
    assert leaf[3].op_str == 'dword ptr [rax], ecx' and leaf[5].op_str == 'rax, 0x10'
    assert leaf[8].op_str == 'eax, eax'
    table,end=rip(leaf[0]),rip(leaf[1]);assert end-table == 48
    rows=struct.unpack_from('<12I',n['binary'],n['offset'](table))
    assert rows == (1495,909,39191,187803,1511,944,40043,193335,1539,1037,41105,203920)

    load=ins('?LoadFromDB@Player@@')
    first,seed=sites(load,'?PrepareScenarioLogin@Player@@')
    find_route,=sites(load,'?Find@WarriorScenarioRoute@@')
    questloads=sites(load,'?_LoadQuestStatus@Player@@');assert len(questloads)==3
    early,warlock,normal=questloads
    target_candidates=sites(load,'?GetSpellTargetPosition@SpellMgr@@')
    lookup,=[i for i in target_candidates if early < i < seed]
    validate,=[i for i in sites(load,'?IsValidMapCoord@MapManager@@') if lookup < i < seed]
    assert first < find_route < early < lookup < validate < seed < warlock < normal
    assert load[first-3].op_str == 'r8d, dword ptr [rsp + 0x54]'
    assert load[seed-3].op_str == 'r8d, r8d' and load[seed-3].mnemonic == 'xor'
    assert load[seed-2].op_str == 'edx, ebx' and load[seed-1].op_str == 'rcx, r12'
    unlock=find(load,'ecx, 8',first-20,'bt');assert unlock < first and load[unlock+1].mnemonic == 'jb'
    login=find(load,inherited_operand('dword ptr [r12 + 0x6bd8], 0'),first,'cmp')
    cls=find(load,'al, 1',login,'cmp')
    assert first < login < cls < find_route
    skip=load[login+1].operands[0].imm
    assert load[login+1].mnemonic == 'jne' and load[cls+1].mnemonic == 'jne' and load[cls+1].operands[0].imm == skip
    assert load[find_route+2].op_str == 'rax, rax' and load[find_route+3].operands[0].imm == skip
    assert find(load,'r8d, 6',find_route,'mov') < early
    assert load[early+1].op_str == 'byte ptr [rsp + 0x50], 1'
    status=find(load,'eax, 3',early,'cmp');assert status < lookup and load[status+1].operands[0].imm == skip
    assert find(load,'edx, dword ptr [r15 + 8]',early) < status
    assert find(load,'edx, dword ptr [r15 + 0xc]',status) < lookup
    null=find(load,'rax, rax',lookup,'test');mapping=find(load,'dword ptr [rax], ebx',null,'cmp')
    assert load[null+1].operands[0].imm == skip and load[mapping+1].operands[0].imm == skip
    zero=find(load,'xmm0, xmm6',mapping,'ucomiss');zero=find(load,'xmm0, xmm6',zero+1,'ucomiss')
    assert load[zero+2].mnemonic == 'je' and load[zero+2].operands[0].imm < load[seed].address+128
    assert load[validate+2].op_str == 'al, al' and load[validate+3].mnemonic == 'je'
    assert load[zero+2].operands[0].imm > load[seed].address and load[validate+3].operands[0].imm > load[seed].address
    for text in ('dword ptr [rsp + 0x260], ebx','eax, dword ptr [r15 + 4]','dword ptr [rsp + 0x268], r13d','qword ptr [rsp + 0x270], r13','rax, qword ptr [rsp + 0x80]','xmmword ptr [rax], xmm0','xmmword ptr [rax + 0x10], xmm2','qword ptr [rax + 0x20], xmm4'):
        assert validate < find(load,text,validate) < seed
    assert find(load,'r13d, r13d',validate,'xor') < find(load,'dword ptr [rsp + 0x268], r13d',validate)
    resume_ptr=find(load,inherited_operand('rbx, [r15 + 0x6bb0]'),0,'lea')
    assert load[resume_ptr+1].op_str == 'qword ptr [rsp + 0x80], rbx' and resume_ptr < first
    transport=next(i for i in sites(load,'?Reset@TransportInfo@MovementInfo@@') if seed < i < seed+24)
    assert seed < transport < warlock
    selected=find(load,'dword ptr [rax], 0',seed,'cmp')
    assert selected < transport and load[selected+1].mnemonic == 'je' and load[selected+1].operands[0].imm > load[transport].address
    assert find(load,'qword ptr [rsp + 0x98], r13',transport,'mov') > transport
    assert load[warlock+1].op_str == 'byte ptr [rsp + 0x50], 1'
    once=find(load,'byte ptr [rsp + 0x50], 0',normal-20,'cmp')
    assert load[once+1].mnemonic == 'jne' and load[once+1].operands[0].imm == load[normal+1].address
    rewarded,=sites(load,'?_LoadQuestStatusRewarded@Player@@');assert normal < rewarded

    prepare=ins('?PrepareScenarioLogin@Player@@')
    assert any(o.mnemonic=='lea' and any(a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP for a in o.operands) and rip(o)==table for o in prepare)
    bad=find(prepare,'ecx, dword ptr [rax + 4]',0,'cmp')
    assert prepare[bad+1].mnemonic == 'je'
    assert bad < find(prepare,inherited_operand('qword ptr [rdi + 0x6bd8], rsi'),bad,'mov') < find(prepare,inherited_operand('word ptr [rdi + 0x6ba8], si'),bad,'mov')

    admit=ins('?CanResumeScenarioLogin@Player@@')
    assert any(o.mnemonic=='lea' and any(a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP for a in o.operands) and rip(o)==table for o in admit)
    quest,=sites(admit,'?GetQuestRewardStatus@Player@@');statuses=sites(admit,'?GetQuestStatus@Player@@');assert len(statuses)==2
    scenario=find(admit,'esi, dword ptr [rbx + 4]',0,'cmp')
    reject=admit[scenario+1].operands[0].imm
    assert scenario < quest < statuses[0] < statuses[1]
    assert admit[quest+1].op_str == 'al, al' and admit[quest+2].operands[0].imm == reject
    assert admit[statuses[0]+1].op_str == 'eax, 3' and admit[statuses[1]+1].op_str == 'eax, 1'
    assert admit[statuses[1]+2].operands[0].imm == reject
    assert find(admit,inherited_operand('eax, dword ptr [rdi + 0x6bd8]'),0,'mov') > statuses[1]
    print('PASS selected actual pre-transport Warrior login:three native map/scenario/quest/spell rows,existing receipt first,unlock/class/map/active quest/native-source admission,zero old instance+boot and explicit Prepare(map,0),transport reset only after selected recovery;early/Warlock status flag skips later duplicate load;rewarded rows load normally and final admission rechecks exact route/active-complete/unrewarded quest. Source132/native138 and full client/persisted-character traversal remain separate.')
