"""Full completion/input/admission methods; queue comparator/consent projections named explicitly."""
from pathlib import Path
import json,re,subprocess,sys
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True)
def source(path):
 if mode=='baseline':return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
 return(ROOT/path).read_text(encoding='utf-8-sig')
pb=source('src/server/game/PetBattle/PetBattle.cpp');ph=source('src/server/game/Handlers/BattlePetHandler.cpp');ps=source('src/server/game/PetBattle/PetBattleSystem.cpp')
text='\n'.join([block(pb,r'void PetBattle::Finish\('),block(ph,r'void WorldSession::HandlePetBattleInput\('),block(ps,r'eBattlePetRequests PetBattleSystem::CanPlayerEnterInPetBattle\('),block(ps,r'PetBattleRequest\* PetBattleSystem::CreateRequest\('),block(ps,r'void PetBattleSystem::RemoveRequest\(')])
labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b|\bBATTLEPET_STATE_\w+',text))|{'PETBATTLE_STATUS_RUNNING','BATTLEPET_STATE_Is_Dead','PETBATTLE_TYPE_PVP_DUEL'}
fixed={'MAX_PETBATTLE_TEAM':2,'MAX_PETBATTLE_SLOTS':3,'BATTLEPET_MAX_LEVEL':25,'PETBATTLE_TEAM_1':0,'PETBATTLE_TEAM_2':1,'PETBATTLE_PVE_TEAM_ID':1,'PETBATTLE_NULL_ID':0,'PETBATTLE_NULL_SLOT':-1,'INTERACTION_DISTANCE':5,'PETBATTLE_ACTION_REQUEST_LEAVE':0,'PETBATTLE_ACTION_CAST':1,'PETBATTLE_ACTION_SWAP_OR_PASS':2,'PETBATTLE_ACTION_CATCH':3,'PETBATTLE_ACTION_LEAVE_PETBATTLE':4}
for n in sorted(labels-set(fixed)):fixed[n]=len(fixed)+100
fixed['PETBATTLE_REQUEST_OK']=0
(out/'Labels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n')
queue=block(ps,r'void PetBattleSystem::Update\(')
sort=re.search(r'std::sort\(possibleOpponent.begin\(\), possibleOpponent.end\(\), (\[\].*?\n\s*\})\);',queue,re.S)[1]
consent='\n'.join(line.strip()for line in queue.splitlines()if 'ProposalAnswer = LFBAnswer::LFB_ANSWER_PENDING' in line)
text+='\nstatic bool QueueBefore(LFBTicket*a,LFBTicket*b){return ('+sort+')(a,b);}\nstatic void NewConsent(LFBTicket*l_Left,LFBTicket*l_Right){'+consent+'}\n'
(out/'Selected.inc').write_text(text)
(out/'cases.json').write_text(json.dumps([e['key']for e in PLAN['changes']if e['batch']==1]))
print('Prepared full actual Finish, input, admission, owning request methods; queue ordering/consent are explicit source projections.')
