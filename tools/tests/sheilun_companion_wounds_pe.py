"""Native two-class Sheilun final-damage/applied-wound consumers."""
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats


def verify(n):
    ins=n['instructions']; registered=ins('?AddSC_Sheylun@@')
    # The second named CreatureScript is the shared Shado-Pan class, not an
    # unreferenced test factory. The first registration is proved by intro PE.
    name=registered[39];at=name.address+name.size+name.operands[1].mem.disp
    assert n['binary'][n['offset'](at):n['offset'](at)+24]==b'npc_sheylun_small_events'
    item=registered[47];at=item.address+item.size+item.operands[1].mem.disp
    assert registered[48].op_str=='qword ptr [rbx], rax'
    assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](at+0x50))[0])==n['symbol']('?GetAI@npc_sheylun_small_events@@')
    for role,wait,pending,groups in [('npc_tazhan_chu',0x5a,0x10e,(11,12)),('npc_sheylun_small_events',0x58,0xe0,(0,1))]:
        suffix='@'+role+'AI@'+role+'@@';method=lambda name:'?'+name+suffix
        factory=ins('?GetAI@'+role+'@@')
        tables=[x.address+x.size+x.operands[1].mem.disp for i,x in enumerate(factory[:-1])
                if x.mnemonic=='lea' and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP
                and factory[i+1].op_str=='qword ptr [rsi], rax']
        assert len(tables)==1;table,=tables
        for name,slot in [('DamageTakenAfterScaling',0xa8),('OnHealApplied',0x288),('UpdateLifecycle',0x110),('Reset',0x28)]:
            assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table+slot))[0])==n['symbol'](method(name))
        assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table+0xb0))[0])==n['symbol']('?HealReceived@UnitAI@@')
        raw=n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table+0xa0))[0])
        if role=='npc_tazhan_chu':
            assert raw==n['symbol'](method('DamageTaken'))
            body=[x for x in ins(method('DamageTaken')) if x.mnemonic!='int3']
            assert [(x.mnemonic,x.op_str) for x in body]==[('shr','dword ptr [r8], 1'),('ret','')]
        else:assert raw==n['symbol']('?DamageTaken@UnitAI@@')
        assert any(x.mnemonic=='mov' and x.op_str==f'byte ptr [rsi + {pending:#x}], bpl' for x in factory)
        body=ins(method('DamageTakenAfterScaling'))
        health,=sites(n,body,'?GetHealth@Unit@@QEBA_KPEAV1@@')
        assert health==8 and body[9].op_str=='rbx, rax' and body[10].mnemonic=='jb'
        assert not any(x.op_str.startswith(('rdx,','edx,')) for x in body[:health]), 'Original attacker argument overwritten'
        assert body[12].op_str=='dword ptr [rsi], eax' and body[11].op_str=='eax, eax'
        assert body[13].op_str==f'byte ptr [rdi + {wait:#x}], al'
        assert body[14].mnemonic=='jne' and body[14].operands[0].imm==body[39].address==body[10].operands[0].imm
        assert body[21].op_str==f'byte ptr [rdi + {pending:#x}], al'
        assert body[22].op_str==f'byte ptr [rdi + {wait:#x}], 1'
        aura,=sites(n,body,'?AddAura@Unit@@');talk=sites(n,body,'?Talk@CreatureAI@@')
        assert aura==24 and talk==[30,36] and 204237 in values(body[:aura])
        assert body[18].op_str=='r8, rcx' and body[38].op_str=='dword ptr [rax + 0x3140], 8'
        for index,group in zip(talk,groups):
            window=body[index-5:index]
            assert any((x.mnemonic,x.op_str)==(('xor','edx, edx') if group==0 else ('mov',f'dl, {group:#x}' if group>=10 else f'dl, {group}')) for x in window)
        assert len([x for x in body if x.mnemonic=='call'])==4
        heal=ins(method('OnHealApplied'));life=ins(method('UpdateLifecycle'));reset=ins(method('Reset'))
        observed,=[i for i,x in enumerate(heal) if x.mnemonic=='mov' and x.op_str==f'byte ptr [rcx + {pending:#x}], 1']
        assert heal[observed-2].op_str==f'byte ptr [rcx + {wait:#x}], 0'
        assert heal[observed-1].mnemonic=='je' and heal[observed-1].operands[0].imm==heal[observed+1].address
        if role=='npc_sheylun_small_events':
            assert [(x.mnemonic,x.op_str) for x in heal if x.mnemonic!='int3']==[
                ('cmp',f'byte ptr [rcx + {wait:#x}], 0'),('je',heal[1].op_str),
                ('mov',f'byte ptr [rcx + {pending:#x}], 1'),('ret','')]
        else:assert observed<sites(n,heal,method('IntroHealScenario'))[0]
        removal,=sites(n,life,'?RemoveAura@Unit@@');recover=life[:removal+1]
        assert 204237 in values(recover) and 40.0 in floats(n,recover)
        for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetHealthPct@Unit@@'):
            assert len(sites(n,recover,target))==1,target
        alive,=[i for i,x in enumerate(recover) if x.mnemonic=='cmp' and x.op_str in ('dword ptr [rcx + 0xa18], 0','dword ptr [rcx + 0xa18], r14d')]
        assert recover[alive+1].mnemonic=='jne' and recover[alive+1].operands[0].imm>recover[removal].address
        clear,=[i for i,x in enumerate(recover) if x.mnemonic=='mov' and x.op_str in
                (f'byte ptr [rdi + {wait:#x}], r14b',f'byte ptr [rbx + {wait:#x}], 0')]
        assert clear<removal and any(x.mnemonic=='and' and x.op_str=='dword ptr [rax + 0x3140], 0xfffffff7' for x in recover[clear:])
        consumed,=[i for i,x in enumerate(recover[:clear]) if x.mnemonic=='mov' and x.op_str in (f'byte ptr [rcx + {pending:#x}], r14b',f'byte ptr [rcx + {pending:#x}], 0')]
        assert consumed<sites(n,recover,'?IsInWorld@Object@@')[0]
        comparison,=[i for i,x in enumerate(recover) if x.mnemonic=='comiss']
        assert recover[comparison+1].mnemonic=='ja' and recover[comparison+1].operands[0].imm>recover[removal].address
        assert any(x.mnemonic=='mov' and x.op_str in (f'byte ptr [rbx + {pending:#x}], 0',f'byte ptr [rcx + {pending:#x}], 0') for x in reset)
        for selected in (body,heal,recover):
            for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?CastSpell@Unit@@'):
                assert not sites(n,selected,forbidden)
        print(f'PASS native {role}: registered actual factory/vtables/raw modifier/final target-health lethal latch, original204237/speech/stun, final-heal pending marker and alive/world/40pct lifecycle recovery, reset cancellation; no progression/health grant')
