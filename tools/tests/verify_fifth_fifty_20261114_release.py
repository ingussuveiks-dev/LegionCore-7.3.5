"""New50 paths plus retained200 proofs with chronological source certificates."""
from pathlib import Path
import hashlib,re,runpy,struct
from capstone import CS_OP_IMM,CS_OP_REG,CS_OP_MEM
from ashbringer_storage_pe import reaches
from prepare_fifth_fifty_20261114 import verify,PLAN,ROOT
import prepare_fourth_fifty_20261114 as fourth
import prepare_more_fifty_20261114 as third
import prepare_next_fifty_20261114 as second
import prepare_five_ten_20261114 as first
verify()
def certify(target,plans):
    verify()
    for path,record in target.PLAN['files'].items():
        value=(ROOT/path).read_text(encoding='utf-8-sig')
        for plan in plans:
            for row in reversed(plan['changes']+plan['extra_edits']):
                if row['path']==path:
                    assert value.count(row['new'])==1,(path,row.get('key'))
                    value=value.replace(row['new'],row['old'],1)
        assert hashlib.sha256(value.encode()).hexdigest()==record['after_sha256'],('Retained source certificate',path)
fourth.verify=lambda:certify(fourth,[PLAN])
p=runpy.run_path(str(Path(__file__).with_name('verify_fourth_fifty_20261114_release.py')),init_globals={
 'retained_third_source_certificate':lambda:certify(third,[PLAN,fourth.PLAN]),
 'retained_second_source_certificate':lambda:certify(second,[PLAN,fourth.PLAN,third.PLAN]),
 'retained_first_source_certificate':lambda:certify(first,[PLAN,fourth.PLAN,third.PLAN,second.PLAN])})
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
for row in PLAN['changes']:
    if row['kind']=='corehelper':prefix='?'+row['method']+'@ChatHandler@@'
    elif row['kind']=='coreargs':prefix='?Initialize@CommandArgs@@'
    else:
        source=(ROOT/row['path']).read_text(encoding='utf-8-sig');klass=re.search(r'class (\w+)\s*:\s*public CommandScript',source)[1]
        prefix='?'+row['method']+'@'+klass+'@@'
        if row.get('kind_override')=='sharedcommandhelper':
            assert sites(ins('?HandleCharacterDeletedListCommand@'+klass+'@@'),prefix),row['case']
        else:assert n['symbol'](prefix)in refs(ins('?GetCommands@'+klass+'@@')),row['case']
    body=ins(prefix)
    assert body and not sites(body,'atoi')and not sites(body,'atol')and not sites(body,'atoul')and not sites(body,'strtoull'),('Stale coercion',row['case'])
    print('LINKED',row['case'],len(body),'instructions')
npc=ins('?HandleNpcAddCommand@npc_commandscript@@');create,=sites(npc,'?CreateNPCPassenger@Transport@@');cleanup,=sites(npc,'?DeleteCreatureData@ObjectMgr@@')
# SaveToDB is virtual. Derive its actual offset from this executable's Creature
# vtable, then track the loaded function register to its indirect call.
map_text=(n['args'].release/'worldserver.map').read_text(errors='replace')
tables=re.findall(r'\s+\w+:\w+\s+\?\?_7Creature@@6BUnit@@@\s+([0-9A-Fa-f]{16})',map_text);assert len(tables)==1
table=int(tables[0],16);target=n['symbol']('?SaveToDB@Creature@@UEAAXI_KI@Z')
entries=struct.unpack_from('<84Q',n['binary'],n['offset'](table));slot,=[index*8 for index,address in enumerate(entries)if address and n['resolve'](address)==n['resolve'](target)]
saved=[]
for at,x in enumerate(npc):
    if x.mnemonic!='mov'or x.operands[0].type!=CS_OP_REG or x.operands[1].type!=CS_OP_MEM or x.operands[1].mem.disp!=slot:continue
    register=x.operands[0].reg
    for k in range(at+1,min(at+24,len(npc))):
        call=npc[k]
        if call.mnemonic=='call'and call.operands[0].type==CS_OP_REG and call.operands[0].reg==register:saved.append(k);break
assert create<cleanup and saved,'Actual virtual persistence sites'
# A failed native CreateNPCPassenger result must branch to its owned-record cleanup.
guards=[]
for at in range(create+1,min(create+8,len(npc)-1)):
    test,branch=npc[at:at+2]
    if test.mnemonic!='test'or test.operands[0].type!=CS_OP_REG or branch.mnemonic not in('je','jne'):continue
    reject=branch.operands[0].imm if branch.mnemonic=='je'else npc[at+2].address
    assert reaches(npc,reject,npc[cleanup].address)
    assert all(not reaches(npc,reject,npc[s].address)for s in saved),'Null passenger reaches persistence'
    guards.append(at)
assert guards,'Actual passenger null-result branch'
packet=ins('?HandleDebugSendSpellFailCommand@debug_commandscript@@')
assert sites(packet,'?Write@CastFailed@Spells@WorldPackets@@')and not sites(packet,'?WriteBit@ByteBuffer@@'),'Native7.3.5 packet serializer'
args=ins('?Initialize@CommandArgs@@')
# The optimizer branches to the indexed read only for index<size; rejection
# falls through vector destruction and return. Verify that CFG/dataflow.
capacity=[]
for at in range(1,len(args)-2):
    compare,branch=args[at:at+2]
    if compare.mnemonic!='cmp'or compare.operands[0].type!=CS_OP_REG or compare.operands[0].size!=8 or branch.mnemonic!='jb':continue
    target=next((x for x in args if x.address==branch.operands[0].imm),None)
    if target is None or target.mnemonic!='mov':continue
    indexed=[o for o in target.operands if o.type==CS_OP_MEM and o.mem.index==compare.operands[0].reg and o.mem.scale==4]
    if not indexed:continue
    assert args[at-1].mnemonic=='sar'and args[at-1].operands[-1].type==CS_OP_IMM and args[at-1].operands[-1].imm==2
    assert not reaches(args,args[at+2].address,target.address),'Excess signature argument reaches indexed read'
    capacity.append(at)
assert len(capacity)==1,'Actual64-bit signature capacity guard before enum read'
for method,klass in [('HandleAuraChargesCommand','misc'),('HandleDebugBattlePetRequestFailedCommand','debug'),('HandleDebugSendLFGListStatusUpdate','debug')]:
    body=ins('?'+method+'@'+klass+'_commandscript@@')
    assert sites(body,'??$ParseCommandInteger@E@Trinity@@')or any(o.type==CS_OP_IMM and o.imm in(25,255,256)for x in body for o in x.operands),'Native byte parser '+method
deletion=ins('?DeleteOldCharacters@Player@@SAXI@Z')
assert any(x.mnemonic=='imul'and x.operands[0].size==8 and any(o.type==CS_OP_IMM and o.imm==86400 for o in x.operands)for x in deletion),'Native64-bit retention multiplication'
print('PASS new50 production registrations/shared functions,native passenger null cleanup CFG,7.3.5 CastFailed writer,typed signature capacity and byte boundaries;retained200 PE checks complete. Client/whole36 remain separate.')
