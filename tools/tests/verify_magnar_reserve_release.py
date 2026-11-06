"""Verify selected native reserve recovery, keeping the finale proof separate."""
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

def check(native, instructions):
    suffix='@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
    def calls(prefix, dependency):
        targets={native['resolve'](address) for name,address in native['symbols'].items() if name.startswith(dependency)}
        return [index for index,entry in enumerate(instructions(prefix)) if entry.mnemonic in ('call','jmp') and entry.operands[0].type==CS_OP_IMM and native['resolve'](entry.operands[0].imm) in targets]
    update='?UpdateAI'+suffix
    reserve='?UpdateReserve'+suffix
    roster='?SummonTrash'+suffix
    if not any(name.startswith(reserve) for name in native['symbols']):
        reserve=update  # MSVC inlines the guarded recovery into actual UpdateAI.
    launch='?LaunchEventWave'+suffix
    death='?SummonedCreatureDies'+suffix
    factory='?GetAI@npc_magnar_icebreaker@@'
    ctor='??0npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
    factory_body=instructions(factory)
    if calls(factory,ctor):
        factory_body+=instructions(ctor)
    tables=set()
    for entry in factory_body:
        for operand in entry.operands:
            if entry.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                offset=native['offset'](entry.address+entry.size+operand.mem.disp)
                if struct.unpack_from('<Q',native['binary'],offset+0x158)[0]==native['symbol'](death):
                    tables.add(offset)
    assert len(tables)==1, 'Actual factory does not select repaired death controller'
    table=next(iter(tables))
    assert struct.unpack_from('<Q',native['binary'],table+0x48)[0]==native['symbol']('?DoAction'+suffix)
    assert struct.unpack_from('<Q',native['binary'],table+0x128)[0]==native['symbol']('?EnterCombat'+suffix)
    assert struct.pack('<Q',native['symbol'](update)) in native['binary'][table:table+0x400]
    assert calls(update,roster)[0] < calls(update,'?UpdateVictim@CreatureAI@@')[0]
    death_dispatch=[index for index,entry in enumerate(instructions(reserve)) if entry.mnemonic=='call' and entry.operands[0].type==CS_OP_MEM and entry.operands[0].mem.disp==0x158]
    decoded=instructions(reserve)
    death_dispatch += [index for index,entry in enumerate(decoded) if entry.mnemonic=='call' and entry.op_str=='r9' and
                       decoded[index-2].op_str=='r9, qword ptr [rcx + 0x158]' and
                       any(before.mnemonic=='cmp' and before.op_str=='dword ptr [rax + 0xa18], 0' for before in decoded[max(0,index-9):index])]
    assert death_dispatch or calls(reserve,death), 'Missing selected native summon death recovery dispatch'
    for prefix, dependencies in ((reserve,('?GetEncounterPlayer'+suffix,roster,launch)),
                                  (roster,('?GetCreature@Map@@','?SummonCreature@WorldObject@@','?RemoveFlag@Object@@','?AdvanceWaveIfReady'+suffix)),
                                  (launch,('?GetCreature@Map@@','?IsInWorld@Object@@','?AdvanceWaveIfReady'+suffix)),
                                  (death,('?GetGuidValue@Object@@','?AdvanceWaveIfReady'+suffix)),
                                  ('?DoAction'+suffix,(roster,'?GetEncounterPlayer'+suffix)),
                                  ('?Reset'+suffix,('?DespawnAll@SummonList@@',))):
        for dependency in dependencies:
            assert calls(prefix,dependency), 'Missing actual reserve dependency:'+prefix+' -> '+dependency
    values={v.imm for x in instructions(roster) for v in x.operands if v.type==CS_OP_IMM}
    assert {97123,94707,94813,191821} <= values
    values={v.imm for x in instructions(reserve) for v in x.operands if v.type==CS_OP_IMM}
    assert {96034,1495,1000} <= values
    for prefix in (reserve,roster):
        for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
            assert not calls(prefix,forbidden), 'Recovery itself grants progress/items'
    print('PASS linked Magnar UpdateAI invokes scoped1000ms reserve recovery before victim gates; actual start/roster/native summon/GUID/death/partial-launch/reset paths present, original entries and191821 release retained. Recovery grants no item or progression; actual ASAN batch/death/native metadata and client checks remain distinct.')
