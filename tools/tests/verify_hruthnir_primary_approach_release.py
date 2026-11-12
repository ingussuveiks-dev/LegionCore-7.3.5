from linked_player_abi import inherited_operand
"""Select the original point6 intent/physical arrival repair in Release."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(n,ins):
    hr='@npc_hrutnirAI@npc_hrutnir@@'
    def sites(body,prefix):
        targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type==CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]
    def find(body,text,mnemonic=None):
        return next(i for i,o in enumerate(body) if o.op_str==text and (mnemonic is None or o.mnemonic==mnemonic))
    def has(body,*texts):
        for text in texts:find(body,text)
    def constants(body):
        return {n['binary'][n['offset'](o.address+o.size+a.mem.disp):n['offset'](o.address+o.size+a.mem.disp)+4] for o in body for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP}
    action=ins('?DoAction'+hr);approach,=sites(action,'?StartMagnarChamberApproach'+hr)
    claim=find(action,'byte ptr [rbx + 0xa5], 1','mov')
    talk=next(i for i in sites(action,'?Talk@CreatureAI@@') if claim<i<approach)
    assert claim<talk<approach
    has(action[claim:approach],'dl, 0xe')
    assert not sites(action,'?MovePoint@MotionMaster@@'), 'Original point6 bypasses the new intent owner'
    helper=ins('?StartMagnarChamberApproach'+hr)
    move,=sites(helper,'?MovePoint@MotionMaster@@')
    record=find(helper,'qword ptr [rax + 0xb8]','call')
    intent=find(helper,'qword ptr [rsi + 0x18c], 0xc','mov')
    assert intent<record<move
    has(helper[:intent],'edx, 0x42a','r8d, 0x429',inherited_operand('qword ptr [rdi + 0x3a88], 0x5835a'),'dword ptr [rcx + 0x1c], 0x1b57')
    assert sites(helper[:intent],'??8ObjectGuid@@')
    assert {struct.pack('<f',x) for x in (4884.71,-77.9149,60.9946,25.0)}<=constants(helper[:intent]), 'Original entrance recovery was overwritten'
    entrance=next(i for i,o in enumerate(helper[:intent]) if o.mnemonic=='comiss')
    assert helper[entrance+1].mnemonic=='jbe' and helper[entrance+1].operands[0].imm>helper[record].address
    has(helper[intent-2:record],'r8d, ebx','edx, 0x42b','byte ptr [rsi + 0x194], 0')
    assert helper[record+1].op_str=='qword ptr [rdi + 0x9c0], rsi' and helper[record+2].mnemonic=='jne'
    assert helper[record+2].operands[0].imm>helper[move].address
    has(helper[record:move],'dword ptr [rsi + 0x18c], 0xc','dword ptr [rsi + 0xa8], ebx','edx, 0x42a','r8d, 0x429','dword ptr [rcx + 0x1c], 0x1b57')
    assert sites(helper[record:move],'?IsMagnarOfferContext'+hr) and sites(helper[record:move],'?HasMagnarChamberRequest'+hr) and sites(helper[record:move],'??9ObjectGuid@@')
    has(helper[move-12:move],'edx, 6','byte ptr [rsp + 0x28], 1')
    assert {struct.pack('<f',x) for x in (4763.73,340.87,-37.19)}<=constants(helper[move-12:move])
    assert all(o.mnemonic in ('mov','add','pop','ret') for o in helper[move+1:]), 'AI accessed after native movement boundary'
    body=ins('?MovementInform'+hr);delegates=sites(body,'?OnMagnarRouteArrival'+hr)
    assert len(delegates)==2
    first,second=delegates
    has(body[:first],'edx, 8','r8d, edx')
    has(body[first:second],'edi, 6','dword ptr [rcx + 0x18c], 0xc')
    assert body[second+1].op_str=='qword ptr [rsi + 0x9c0], rbx' and body[second+2].mnemonic=='jne'
    assert body[second+2].operands[0].imm==body[first+1].operands[0].imm
    world,=sites(body[second:],'?IsInWorld@Object@@');world+=second
    assert body[world+1].op_str=='al, al' and body[world+2].mnemonic=='jne'
    dispatch=next(i for i,o in enumerate(body) if o.address==body[world+2].operands[0].imm)
    assert body[dispatch].mnemonic=='dec' and body[dispatch].op_str=='edi'
    # The full route certificate separately proves current909/3/epoch/GUID/
    # phase, ordered2yd physical arrival and tail SetData before any offer.
    arrival=ins('?OnMagnarRouteArrival'+hr)
    assert any(o.mnemonic=='inc' and o.op_str=='dword ptr [rdi + 0x18c]' for o in arrival)
    for section in (helper,arrival):
        for forbidden in ('?NearTeleportTo@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?CreateConversation@','?DoAction','?Talk@CreatureAI@@'):
            assert not sites(section,forbidden)
    print('PASS selected original approach: current DoAction2 claim/Talk14 records native leg12 outside original entrance under earned receipt/fullGUID/DB/phase; callback AI/epoch/receipt revalidated before unchanged point6 move. Actual point6 delegates ordered native arrival13 before ordinary offer; deletion/world guards precede continuation. Full13s/cached menu/point7/497/6s remain independently certified; no progression/teleport grant.')


if __name__=='__main__':
    p=runpy.run_path(str(Path(__file__).with_name('verify_magnar_intro_release.py')))
    check(p['native'],p['instructions'])
