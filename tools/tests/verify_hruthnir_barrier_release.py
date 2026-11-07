"""Bind native open33309/reload restoration and callback barriers to Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM


def check(n,ins):
    suffix='@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'
    def sites(body,prefix):
        targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i,op in enumerate(body) if op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_IMM and n['resolve'](op.operands[0].imm) in targets]
    def has(body,*operands):
        for operand in operands:assert any(op.op_str==operand for op in body),'Missing selected barrier operand: '+operand
    name='?RestoreMagnarBarrier'+suffix;restore=ins(name)
    opens,=sites(restore,'?SetGoState@GameObject@@')
    phases,=[i for i,op in enumerate(restore) if op.mnemonic=='call' and op.op_str=='r9']
    lookup,=sites(restore,'?GetGameObject@Map@@')
    has(restore[:phases],'byte ptr [rcx + 0x356], 0','byte ptr [rcx + 0x357], 0',
        'qword ptr [rdx + 8], rax','dword ptr [rdx + 0xe0], 0x5d7','eax, 0x3b8b9','rax, 0x20c1a',
        'rdx, [rsi + 0x346]','eax, eax','dword ptr [rax + 0x1c], 0x1b57',
        'eax, 3','eax, 4','eax, 0x38d','eax, ebx','dl, 1')
    assert sites(restore,'?GetGoType@GameObject@@') and sites(restore,'??9ObjectGuid@@')
    assert sites(restore,'?GetStepCount@Scenario@@') and len(sites(restore,'?GetCurrentStep@Scenario@@'))==2
    assert any(op.mnemonic=='jb' for op in restore[:phases]) and any(op.mnemonic=='ja' for op in restore[:phases]),'Scenario stage bounds missing'
    has(restore[phases-6:phases],'r8b, 1','edx, 4','r9, qword ptr [rcx + 0xa0]')
    assert phases<lookup<opens
    guard=next(i for i,op in enumerate(restore) if op.mnemonic=='cmp' and op.op_str=='ebp, dword ptr [rsi + 0x35c]')
    assert phases<guard<lookup and restore[guard+1].mnemonic=='jne' and restore[guard+1].operands[0].imm>restore[opens].address
    has(restore[lookup:opens],'rdx, [rsi + 0x346]','eax, 0x3b8b9','dword ptr [rcx + 0x1c], 0x1b57','dword ptr [rdi + 0x3bc], 4','eax, 1')
    assert restore[opens-2].mnemonic=='xor' and restore[opens-2].op_str=='edx, edx', 'Door open state must be native ACTIVE0'
    assert all(op.mnemonic in ('mov','add','pop','ret') for op in restore[opens+1:]),'New post-native-hook controller/GO access'
    for forbidden in ('?EnableCollision@GameObject@@','?UpdateAchievementCriteria@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?NearTeleportTo@','?MovePoint@MotionMaster@@','?DoAction'):
        assert not sites(restore,forbidden),'Barrier helper bypasses native door/progression'
    create=ins('?OnGameObjectCreate'+suffix);created,=sites(create,name)
    has(create[:created],'eax, 0x3b8b9','rax, 0x20c1a','xmmword ptr [rdi + 0x346], xmm0')
    handoff=ins('?TryAdvanceMagnarGuards'+suffix);released,=sites(handoff,name)
    after=handoff[released+1:]
    assert after[0].op_str=='r15d, dword ptr [rbp + 0x35c]' and after[1].mnemonic=='jne'
    assert sites(after,'?GetCreature@Map@@') and sites(after,'?GetScenario@ScenarioMgr@@')
    has(after,'eax, 0x38d','eax, 3','eax, 0x178d4','dword ptr [rdi + 0xa18], 0','byte ptr [rdi + 0x39c5], 0',
        'byte ptr [rdi + 0x858], 0','qword ptr [rdi + 0x9c0], rsi','dword ptr [rax + 0x1c], 0x1b57')
    assert sites(after,'??8ObjectGuid@@'), 'Post-hook original companion full GUID lost'
    action,=[i for i,op in enumerate(after) if op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x48]']
    has(after[action-3:action],'edx, 2','rcx, rsi')
    assert after[1].operands[0].imm>after[action].address
    print('PASS selected Protection barrier: original134170/243897/1495/6999/DOOR/default3-or4/current GUID,real guard+delivery/909 stage3..native terminal; native phase4 then epoch/full-GUID live relookup,only ready->ACTIVE0 via SetGoState last. Original handoff and pre-publication OnCreate call it;post-state hook rechecks epoch/scenario/original companion/phase/current AI before action2. No direct collision disable/credit/teleport grants; native33309 mesh and client traversal separate.')


if __name__=='__main__':
    p=runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_guards_release.py')))
    check(p['native'],p['instructions'])
