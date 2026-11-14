"""Compose retained100 PE proofs with an exact third50 source certificate."""
from pathlib import Path
import hashlib,re,runpy
from capstone import CS_OP_IMM
from ashbringer_storage_pe import reaches
import prepare_next_fifty_20261114 as second
import prepare_five_ten_20261114 as first
from prepare_more_fifty_20261114 import verify,PLAN,ROOT
verify()

def undo(value,path,plan):
    for change in reversed(plan['changes']+plan['extra_edits']):
        if change['path']==path:
            assert value.count(change['new'])==1,(path,change.get('key'))
            value=value.replace(change['new'],change['old'],1)
    return value

def certify_second():
    verify()
    for path,record in second.PLAN['files'].items():
        value=undo((ROOT/path).read_text(encoding='utf-8-sig'),path,PLAN)
        assert hashlib.sha256(value.encode()).hexdigest()==record['after_sha256'],('Retained second50 source',path)

def certify_first():
    certify_second()
    for path,record in first.PLAN['files'].items():
        value=undo(undo((ROOT/path).read_text(encoding='utf-8-sig'),path,PLAN),path,second.PLAN)
        assert hashlib.sha256(value.encode()).hexdigest()==record['after_sha256'],('Retained first50 source',path)

second.verify=certify_second
p=runpy.run_path(str(Path(__file__).with_name('verify_next_fifty_20261114_release.py')),init_globals={'previous_source_certificate':certify_first})
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
for row in PLAN['changes']:
    source=(ROOT/row['path']).read_text(encoding='utf-8-sig')
    klass=re.search(r'class (\w+)\s*:\s*public CommandScript',source)[1]
    prefix='?'+row['method']+'@'+klass+'@@'
    body=ins(prefix)
    if row['method'] not in ('HandleAddDisables','HandleRemoveDisables','_HandleGMTicketResponseAppendCommand'):
        assert n['symbol'](prefix) in refs(ins('?GetCommands@'+klass+'@@')),row['case']
    assert not sites(body,'atoi') and not sites(body,'atol'),('Stale integer conversion',row['case'])
    # uint16/int8 parser specializations may be inlined. Source extraction tests
    # check their full admitted/rejected domains, native probe checks the types.
    print('LINKED',row['case'],len(body),'instructions')

for method in ('HandleDebugModifyHasteCommand','HandleDebugModifyHitCommand','HandleDebugModifyMasteryCommand'):
    body=ins('?'+method+'@debug_commandscript@@');writes=sites(body,'?ApplyRatingMod@Player@@');assert writes
    assert any(o.type==CS_OP_IMM and o.imm in (32767,32768,65535,-32768) for x in body[:writes[0]] for o in x.operands),'Native rating sum bounds'
for stem in ('ShutDown','Restart','IdleRestart','IdleShutDown'):
    body=ins('?HandleServer'+stem+'Command@server_commandscript@@');writes=sites(body,'?ShutdownServ@World@@');assert writes
    parse=sites(body,'??$ParseCommandInteger@H@Trinity@@');assert len(parse)==2
    for at in parse:
        assert body[at+1].mnemonic=='test' and body[at+2].mnemonic in ('je','jne')
        reject=body[at+2].operands[0].imm if body[at+2].mnemonic=='je' else body[at+3].address
        assert all(not reaches(body,reject,body[w].address)for w in writes),'Rejected lifecycle arg reaches shutdown'
    assert any(o.type==CS_OP_IMM and o.imm==125 for x in body for o in x.operands)
skill=ins('?HandleSetSkillCommand@misc_commandscript@@');assert sites(skill,'?SetSkill@Player@@')
phase=ins('?HandleDebugSendSetPhaseShiftCommand@debug_commandscript@@');assert sites(phase,'?SendSetPhaseShift@WorldSession@@')
mail=ins('?HandleSendMailStatus@debug_commandscript@@');assert sites(mail,'?SendMailResult@Player@@')
print('PASS new50 actual production registrations/helpers and linked native rating/lifecycle/skill/phase/mail paths;retained100 PE checks completed under exact chronological inverse-delta certificates. Controlled source/native checks are separate from client traversal.')
