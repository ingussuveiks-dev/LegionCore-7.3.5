"""Selected first-ten command registration and actual repaired native branches."""
from pathlib import Path
import re,runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from prepare_five_ten_20261114 import verify
from ashbringer_storage_pe import reaches
verify()
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,ins=p['native'],p['instructions']


def sites(body,prefix):
    targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
    return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]


def refs(body):
    return [x.address+x.size+o.mem.disp for x in body for o in x.operands if x.mnemonic=='lea' and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP]


commands={
    'go_commandscript':['HandleGoQuestCommand'],
    'wp_commandscript':['HandleWpShowCommand'],
    'account_commandscript':['HandleAccountSetGmLevelCommand'],
    'cast_commandscript':['HandleCastDistCommand'],
    'titles_commandscript':['HandleTitlesSetMaskCommand'],
    'modify_commandscript':['HandleModifyEnergyCommand','HandleModifyRageCommand','HandleModifyRunicPowerCommand'],
}
for klass,methods in commands.items():
    addresses=refs(ins('?GetCommands@'+klass+'@@'))
    for method in methods:
        assert n['symbol']('?'+method+'@'+klass+'@@') in addresses,('Unselected command handler',method)

go=ins('?HandleGoQuestCommand@go_commandscript@@')
for prefix in ('?GetQuestPOIVector@QuestDataStoreMgr@@','?CreateBaseMap@MapManager@@','?GetHeight@Map@@','?GetWaterLevel@Map@@','?CleanupAfterTaxiFlight@Player@@','?SaveRecallPosition@Player@@'):
    assert sites(go,prefix),prefix
pose,=[i for i,x in enumerate(go)if re.fullmatch(r'(r(?:[89]|1[0-5])b), \1',x.op_str) and x.mnemonic=='test']
assert go[pose+1].mnemonic=='je'
teleport,=sites(go,'?TeleportTo@Player@@')
assert not reaches(go,go[pose+1].operands[0].imm,go[teleport].address),'No coherent POI must reject before teleport'
assert go[teleport+1].mnemonic=='jmp','Return actual native teleport result'

wp=ins('?HandleWpShowCommand@wp_commandscript@@')
checks=[i for i,x in enumerate(wp)if x.mnemonic=='test' and x.op_str=='r13, r13']
assert len(checks)>=2
info=checks[1]
assert wp[info+1].mnemonic=='je'
assert any(x.op_str=='eax, 1' for x in wp[info+1:info+9]),'Retain actual VISUAL_WAYPOINT1 check'

account=ins('?HandleAccountSetGmLevelCommand@account_commandscript@@')
parsed,=sites(account,'??$ParseCommandInteger@H@Trinity@@')
security=sites(account,'?GetSecurity@AccountMgr@@')
assert len(security)==2 and parsed<min(security),'Realm parse must precede ACL checks'
assert account[parsed-2].mnemonic=='lea' and account[parsed-2].op_str.startswith('rdx, ')
parsed_slot=account[parsed-2].op_str.split(', ',1)[1]
realm_register,={x.op_str.split(', ',1)[0]for x in account[parsed:security[0]]if x.mnemonic=='mov'and x.op_str.endswith(', dword ptr '+parsed_slot)}
assert re.fullmatch(r'r(?:[89]|1[0-5])d|e[a-z]{2}',realm_register),'Retain parsed realm in an actual32-bit register'
setters=sites(account,'?setUInt32@PreparedStatementBase@@')
assert any([x.op_str for x in account[i-3:i]]==['r8d, '+realm_register,'dl, 1','rcx, rbx'] for i in setters),'DELETE parameter1 must use requested realm,not realm.Id.Realm'
first_token=next(i for i,x in enumerate(account)if x.mnemonic=='test' and x.op_str=='rax, rax')
assert account[first_token+1].mnemonic=='je'
assert not any(reaches(account,account[first_token+1].operands[0].imm,account[i].address)for i in sites(account,'?GetId@AccountMgr@@')+security),'Absent first token must not reach account assignment/ACL'

title=ins('?HandleTitlesSetMaskCommand@titles_commandscript@@')
mask,=[i for i,x in enumerate(title)if x.mnemonic=='cmp' and x.op_str=='ax, 0x3f']
assert title[mask+1].mnemonic=='ja' and title[mask+2].mnemonic=='btr'
assert title[mask+1].operands[0].imm>title[mask+2].address,'Unsigned16-bit bound also rejects negative native MaskID'
assert sites(title,'?SetUInt64Value@Object@@') and sites(title,'?HasTitle@Player@@')

cast=ins('?HandleCastDistCommand@cast_commandscript@@')
map_text=(n['args'].release/'worldserver.map').read_text(errors='replace')
fd=re.search(r'\s_fdclass\s+([0-9a-fA-F]{16})\s+f',map_text)
assert fd
classified,=[i for i,x in enumerate(cast)if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==int(fd[1],16)]
assert cast[classified+1].op_str=='ax, ax' and cast[classified+2].mnemonic=='jg'
cast_site,=sites(cast,'?CastSpell@Unit@@QEAA?AW4SpellCastResult@@MMMI_N')
assert not reaches(cast,cast[classified+2].operands[0].imm,cast[cast_site].address),'NaN/Inf classification must reject before cast'

for name in ('Energy','Rage','RunicPower'):
    body=ins('?HandleModify'+name+'Command@modify_commandscript@@')
    parse,=sites(body,'??$ParseCommandInteger@H@Trinity@@')
    bound,=[i for i,x in enumerate(body)if x.mnemonic=='cmp' and x.op_str=='eax, 0xccccccb']
    assert parse<bound and body[bound+1].mnemonic=='ja'
    maximum,=sites(body,'?SetMaxPower@Unit@@')
    set_power,=sites(body,'?SetPower@Unit@@')
    assert not reaches(body,body[bound+1].operands[0].imm,body[maximum].address)
    lower,=sites(body,'?HasLowerSecurity@ChatHandler@@')
    assert lower<maximum<set_power

soul=ins('?IsSummonedBy@npc_soul_echoes_stalker@@')
random,=sites(soul,'?urand@@')
assert [x.op_str for x in soul[random-2:random]]==['edx, 2','ecx, ecx']
table,=[x.address+x.size+x.operands[1].mem.disp for x in soul[random:random+9]if x.mnemonic=='lea' and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP]
assert struct.unpack_from('<3I',n['binary'],n['offset'](table))==(9038,9039,9176)
assert sites(soul,'?PlayOneShotAnimKit@Unit@@') and 4000 in {o.imm for x in soul for o in x.operands if o.type==CS_OP_IMM}
factory=ins('?GetAI@?$GenericCreatureScript@Unpc_soul_echoes_stalker@@@@')
selected=[]
for address in refs(factory):
    slots=struct.unpack_from('<80Q',n['binary'],n['offset'](address))
    if n['symbol']('?IsSummonedBy@npc_soul_echoes_stalker@@') in slots:selected.append(slots.index(n['symbol']('?IsSummonedBy@npc_soul_echoes_stalker@@'))*8)
assert len(selected)==1
assert any(x.mnemonic=='call' and o.type==CS_OP_MEM and o.mem.disp==selected[0] for x in ins('?InitSummon@TempSummon@@')for o in x.operands),'Native summon dispatcher must select the repaired callback'
print('PASS selected registered first-ten command handlers and Soul Echoes factory/native summon slot;coherent POI failure CFG,requested ACL realm,finite distance,title width,scaled power bounds/security and actual0..2 animation indexing. Source ASAN/native catalog/client scopes remain separate.')
