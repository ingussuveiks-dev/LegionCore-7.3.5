"""Retain150 selected PE checks under exact fourth50 inverse source deltas."""
from pathlib import Path
import hashlib,re,runpy
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from ashbringer_storage_pe import reaches
from prepare_fourth_fifty_20261114 import verify,PLAN,ROOT
import prepare_more_fifty_20261114 as third
import prepare_next_fifty_20261114 as second
import prepare_five_ten_20261114 as first
verify()
def undo(value,path,plan):
    for change in reversed(plan['changes']+plan['extra_edits']):
        if change['path']==path:
            assert value.count(change['new'])==1,(path,change.get('key'))
            value=value.replace(change['new'],change['old'],1)
    return value
def certify(target,plans):
    verify()
    for path,record in target.PLAN['files'].items():
        value=(ROOT/path).read_text(encoding='utf-8-sig')
        for plan in plans:value=undo(value,path,plan)
        assert hashlib.sha256(value.encode()).hexdigest()==record['after_sha256'],('Retained source certificate',path)
def certify_third():certify(third,[PLAN])
def certify_second():certify(second,[PLAN,third.PLAN])
def certify_first():certify(first,[PLAN,third.PLAN,second.PLAN])
third.verify=globals().get('retained_third_source_certificate',certify_third)
p=runpy.run_path(str(Path(__file__).with_name('verify_more_fifty_20261114_release.py')),init_globals={'retained_second_source_certificate':globals().get('retained_second_source_certificate',certify_second),'retained_first_source_certificate':globals().get('retained_first_source_certificate',certify_first)})
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
for row in PLAN['changes']:
    if row['kind']=='chatspellhelper':
        prefix='?extractSpellIdFromLink@ChatHandler@@'
    else:
        source=(ROOT/row['path']).read_text(encoding='utf-8-sig')
        klass=re.search(r'class (\w+)\s*:\s*public CommandScript',source)[1]
        prefix='?'+row['method']+'@'+klass+'@@'
        assert n['symbol'](prefix) in refs(ins('?GetCommands@'+klass+'@@')),row['case']
    body=ins(prefix)
    assert body and not sites(body,'atoi') and not sites(body,'atol') and not sites(body,'atoul'),('Stale selected numeric conversion',row['case'])
    print('LINKED',row['case'],len(body),'instructions')
helper=ins('?extractSpellIdFromLink@ChatHandler@@')
assert sites(helper,'??$ParseCommandInteger@I@Trinity@@')or any(o.type==CS_OP_IMM and o.imm==429496729 for x in helper for o in x.operands),'Native shared unsigned parser guards'
aura=ins('?HandleAuraAmountCommand@misc_commandscript@@');get=sites(aura,'?GetAuraEffect@Unit@@');assert get and any(o.type==CS_OP_IMM and o.imm in(31,32)for x in aura[:get[0]]for o in x.operands),'Native effect32 boundary before array lookup'
explore=ins('?HandleExploreCheatCommand@cheat_commandscript@@');assert sites(explore,'?SetUInt32Value@Object@@')and not sites(explore,'?SetFlag@Object@@'),'Exploration uses replacement masks,not OR0'
step=ins('?HandleScenarioStepCommand@scenario_commandscript@@');count,=sites(step,'?GetStepCount@Scenario@@');write,=sites(step,'?SetCurrentStep@Scenario@@');assert count<write,'Matching live total/bonus step bound before transition'
money=ins('?HandleSendMoneyCommand@misc_commandscript@@')
# MSVC combines !money || money>MAX into unsigned(money-1)>MAX-1.
# Require the actual64-bit dataflow and rejected branch,not just a constant.
mail_writes=sites(money,'?SendMailTo@MailDraft@@');cap_guards=[]
for i in range(1,len(money)-2):
    load=money[i]
    if load.mnemonic!='movabs' or load.operands[-1].type!=CS_OP_IMM or load.operands[-1].imm!=99999999998:continue
    lea,cmp,branch=money[i-1],money[i+1],money[i+2]
    assert lea.mnemonic=='lea'and lea.operands[0].type==CS_OP_REG and lea.operands[1].type==CS_OP_MEM and lea.operands[1].mem.disp==-1 and lea.operands[0].size==8
    assert cmp.mnemonic=='cmp'and cmp.operands[0].reg==lea.operands[0].reg and cmp.operands[1].reg==load.operands[0].reg and cmp.operands[0].size==8
    assert branch.mnemonic=='ja'and branch.operands[0].type==CS_OP_IMM
    assert mail_writes and all(not reaches(money,branch.operands[0].imm,money[w].address)for w in mail_writes),'Rejected zero/above-cap amount reaches mail delivery'
    cap_guards.append(i)
assert len(cap_guards)==1,'Native1..MAX money interval guard'
assert sites(money,'?AddMoney@MailDraft@@')or sites(money,'?SendMailTo@MailDraft@@'),'Native mail delivery retained'
items=ins('?HandleSendItemsCommand@misc_commandscript@@');assert any(o.type==CS_OP_IMM and o.imm==12 for x in items for o in x.operands)and sites(items,'?SendMailTo@MailDraft@@'),'Native12-attachment/mail paths'
itemset=ins('?HandleAddItemSetCommand@misc_commandscript@@');stored,=sites(itemset,'?StoreNewItem@Player@@');notifications=sites(itemset,'?SendNewItem@Player@@');assert notifications
# Default GuidSet/bonus temporaries are destroyed before the source if(!item).
# Track the actual returned pointer across that cleanup instead of assuming
# the null test is adjacent to StoreNewItem.
saved=itemset[stored+1]
assert saved.mnemonic=='mov'and saved.operands[1].type==CS_OP_REG and saved.reg_name(saved.operands[1].reg)=='rax'
pointer=saved.operands[0].reg;assert saved.reg_name(pointer)in('rbx','rbp','rsi','rdi','r12','r13','r14','r15')
null_guards=[]
for at in range(stored+2,len(itemset)-1):
    check=itemset[at];branch=itemset[at+1]
    if check.mnemonic!='test'or check.operands[0].type!=CS_OP_REG or check.operands[0].reg!=pointer or check.operands[1].reg!=pointer or branch.mnemonic not in ('je','jne'):continue
    assert not any(o.type==CS_OP_MEM and o.mem.base==pointer for x in itemset[stored+2:at]for o in x.operands),'Item pointer dereferenced before null guard'
    reject=branch.operands[0].imm if branch.mnemonic=='je'else itemset[at+2].address
    assert all(not reaches(itemset,reject,itemset[w].address)for w in notifications),'Null created item reaches grant notification'
    null_guards.append(at)
assert len(null_guards)==1,'Native failed item creation return before binding/notification'
for method in ('HandleNpcDeleteCommand','HandleNpcWhisperCommand'):
    body=ins('?'+method+'@npc_commandscript@@');assert sites(body,'?GetMaxCounter@ObjectGuid@@')or any(o.type==CS_OP_IMM and o.imm==0xffffffffff for x in body for o in x.operands),'Native40-bit counter guard'
print('PASS new50 registered production paths/shared spell helper,native effect/exploration/step/mail/counter branches and retained150 PE proofs under exact chronological inverse-delta certificates. Actual client/whole36 remain separate.')
