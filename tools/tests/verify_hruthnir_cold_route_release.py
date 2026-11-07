"""Bind the earned cold-entrance corridor to the selected Release methods."""
from pathlib import Path
import runpy, struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

def check(n,ins):
    suffix='@npc_hrutnirAI@npc_hrutnir@@'
    def sites(body,prefix):
        targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i,op in enumerate(body) if op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_IMM and n['resolve'](op.operands[0].imm) in targets]
    def has(body,*operands):
        for operand in operands:assert any(op.op_str==operand for op in body),'Missing selected cold route operand: '+operand
    def constants(body):
        result=set()
        for op in body:
            for operand in op.operands:
                if operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                    offset=n['offset'](op.address+op.size+operand.mem.disp)
                    result.add(n['binary'][offset:offset+4])
        return result
    route=ins('?UpdateMagnarChamberRoute'+suffix)
    point,=sites(route,'?GetMagnarRoutePoint'+suffix)
    path,=sites(route,'?CalculatePath@PathGenerator@@QEAA_NMMM_N')
    move,=sites(route,'?MovePoint@MotionMaster@@')
    has(route[:point],'byte ptr [rcx + 0xa5], 0','byte ptr [rcx + 0x5d], 0','qword ptr [rcx + 0x60], 0','qword ptr [rcx + 0x78], 0',
        'dword ptr [rcx + 0x18c], 0xc','r8d, 0x429','dword ptr [rcx + 0x1c], 0x1b57',
        'dword ptr [rdi + 0x190], 0x3e8','dword ptr [rcx + 0x3140], 0x1409')
    for name in ('IsMagnarOfferContext','HasMagnarChamberRequest','IsMagnarIntroPlayer'):
        assert sites(route[:point],'?'+name+suffix)
    assert sites(route[:point],'??9ObjectGuid@@') and sites(route[:point],'?isInCombat@Unit@@')
    assert sites(route[:point],'?IsMovementPreventedByCasting@Unit@@') and sites(route[:point],'?GetCurrentMovementGeneratorType@MotionMaster@@')
    assert {struct.pack('<f',v) for v in (4884.71,-77.9149,60.9946,25.0)}<=constants(route[:point])
    has(route[point:path],'byte ptr [rsp + 0x20], 0')
    has(route[path:move],'al, 0x3f','al, 1','byte ptr [rdi + 0x194], 0','edx, 8','byte ptr [rsp + 0x28], al')
    assert struct.pack('<f',1.0) in constants(route[path:move])
    compare=next(i for i,op in enumerate(route[path:move],path) if op.mnemonic=='comiss')
    assert route[compare+1].mnemonic=='jb' and route[compare+1].operands[0].imm>route[move].address
    assert not sites(route,'?FindNearestCreature@WorldObject@@'),'Cold corridor still depends on a boss400yd away'
    assert sites(route[move:],'??1PathGenerator@@'),'Native path resources not released'
    getter=ins('?GetMagnarRoutePoint'+suffix)
    has(getter,'eax, 6','dword ptr [rcx + 0x10], 0x966fbc','rax, 0x134','rax, 0x2c','rcx, rax, 0x2c')
    literal={op.operands[-1].imm&0xffffffff for op in getter if op.mnemonic=='mov' and op.operands[-1].type==CS_OP_IMM}
    values=(4864.44,-33.53,48.68,4809.14,164.68,-11.20,4824.81,246.52,-24.29,4822.31,328.56,-30.14,4763.73,340.87,-37.19)
    assert {struct.unpack('<I',struct.pack('<f',v))[0] for v in values}<=literal,'Authored original point1/2/4/5/6 changed'
    body=ins('?MovementInform'+suffix);home=sites(body,'?SetHomePosition@Creature@@')[0];arrival=body[:home-6]
    has(arrival,'edx, 8','ebx, edx','dword ptr [rcx + 0x18c], r15d','r8d, 0x429','dword ptr [rcx + 0x1c], 0x1b57',
        'dword ptr [rdi + 0x18c]','dword ptr [rdi + 0x190], r15d')
    assert sites(arrival,'?IsMagnarOfferContext'+suffix) and sites(arrival,'?HasMagnarChamberRequest'+suffix) and sites(arrival,'??9ObjectGuid@@')
    selected,=sites(arrival,'?GetMagnarRoutePoint'+suffix)
    increment=next(i for i,op in enumerate(arrival) if op.mnemonic=='inc' and op.op_str=='dword ptr [rdi + 0x18c]')
    ordered=next(i for i,op in enumerate(arrival) if i>selected and op.mnemonic=='comiss')
    assert selected<ordered<increment and arrival[ordered+1].mnemonic=='jb'
    assert arrival[ordered+1].operands[0].imm>body[home].address
    assert struct.pack('<f',4.0) in constants(arrival[selected:increment])
    assert arrival[increment+2].mnemonic=='jmp' and arrival[increment+2].operands[0].imm>body[home].address,'Intermediate arrival falls into old combat/offer hooks'
    reoffer=ins('?UpdateMagnarReoffer'+suffix)
    has(reoffer[:20],'eax, dword ptr [rcx + 0x18c]','eax, 0xc')
    update=ins('?UpdateAI'+suffix)
    assert sites(update,'?RestoreMagnarChamber'+suffix)[0]<sites(update,'?UpdateMagnarChamberRoute'+suffix)[0]<sites(update,'?UpdateMagnarReoffer'+suffix)[0]<sites(update,'?UpdateVictim@CreatureAI@@')[0]
    action=ins('?DoAction'+suffix)
    epoch=next(i for i,op in enumerate(action) if op.mnemonic=='cmp' and op.op_str=='dword ptr [rbx + 0xa8], eax')
    assert action[epoch+1].mnemonic=='je'
    retire,=sites(action,'?RemoveMagnarConversation'+suffix)
    has(action[epoch:retire],'dword ptr [rbx + 0xa8], esi','qword ptr [rbx + 0x18c], 0','byte ptr [rbx + 0x194], 0')
    assert action[epoch+1].operands[0].imm>action[retire].address,'New epoch route retirement also resets the same live attempt'
    for section in (route,arrival):
        for forbidden in ('?NearTeleportTo@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?CreateConversation@','?Talk@CreatureAI@@','?DoAction','?MovementInform'):
            assert not sites(section,forbidden),'Cold restoration bypasses original flow'
    print('PASS selected cold corridor: earned current909/3 chamber/epoch/original companion/6999/entrance-only start,eligible participant and1s movement deferrals; actual seven9859004 rows plus original point1/2/4/5/6,normal complete path mask/ordered1yd endpoint/native point8. Only real ordered2yd arrival advances; intermediate notifications return before old hooks,ordinary nearby reoffer owns boss admission/point6/13s offer. A real new1060 epoch retires old point/timer/warning state before conversation cleanup; same attempt preserved. No teleport/progression/dialogue/boss-start grant; native movement/client evidence separate.')

if __name__=='__main__':
    p=runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_chamber_release.py')))['check'](p['native'],p['instructions'])
    check(p['native'],p['instructions'])
