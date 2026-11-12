from pathlib import Path
import json,re,subprocess,sys
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[e for e in PLAN['changes']if e['batch']==3];assert len(rows)==10
def source(path):
 if mode=='baseline':return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['batch_baselines']['3']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
 return(ROOT/path).read_text(encoding='utf-8-sig')
pb=source('src/server/game/PetBattle/PetBattle.cpp');ps=source('src/server/game/PetBattle/PetBattleSystem.cpp');ph=source('src/server/game/Handlers/BattlePetHandler.cpp');pe=source('src/server/game/PetBattle/PetBattleAbilityEffect.cpp');pl=source('src/server/game/Entities/Player/Player.cpp')
text='\n'.join(block(ph,r'void WorldSession::'+n+r'\(')for n in ['HandleBattlePetSummon','HandleBattlePetSetFlags','HandleBattlePetSetSlot','HandlePetBattleRequestUpdate','HandleBattlePetNameQuery'])
text+='\n'+block(pl,r'void Player::UpdateBattlePetCombatTeam\(')+'\n'+block(ps,r'eBattlePetRequests PetBattleSystem::CanPlayerEnterInPetBattle\(')+'\n'+block(ps,r'PetBattleRequest\* PetBattleSystem::CreateRequest\(')+'\n'+block(ps,r'void PetBattleSystem::RemoveRequest\(')+'\n'+block(pe,r'bool PetBattleAbilityEffect::HandleLockActiveAbility\(')
def edit(row,index):return rows[row]['edits'][index]['old'if mode=='baseline'else'new']
text+='\nstatic void NormalizeInitialPet(std::shared_ptr<BattlePetInstance>pet){'+edit(5,0)+'\n'
text+='\nstatic void SelectInitialFront(PetBattle&b,size_t i){auto&Teams=b.Teams;'+edit(5,1)+'}\n'
text+='\nstatic void SnapshotXP(std::shared_ptr<BattlePetInstance>pet){'+edit(6,0)+'}\nstatic bool AwardedXP(std::shared_ptr<BattlePetInstance>pet){struct{bool AwardedXP=false;}petUpdate;'+edit(6,1)+'return petUpdate.AwardedXP;}\n'
text+='\nstruct MeanFixture{int32 _LFBAvgWaitTime=1000;uint32 _LFBNumWaitTimeAvg=1000000000;void Observe(Ticket*ticket){'+edit(7,0)+'}};\n'
if mode=='fixed':text+='\n'+block(ph,r'static void RemoveBattlePetJournalEntry\(')
else:
 old=block(ph,r'void WorldSession::HandleBattlePetDelete\(');assert '_player->_battlePets.erase(packet.BattlePetGUID);'in old
 text+='\nstatic void RemoveBattlePetJournalEntry(Player*player,Guid journalID){player->_battlePets.erase(journalID);}\n'
labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b|\bBATTLEPET_STATE_\w+',text))|{'PETBATTLE_STATUS_RUNNING','BATTLEPET_STATE_Is_Dead','PETBATTLE_TYPE_PVP_DUEL','PETBATTLE_TEAM_CATCH_FLAG_ENABLE_TRAP','BATTLEPET_SPECIES_FLAG_COMPANION','PETBATTLE_TYPE_PVE','PVE_PETBATTLE_TRAINER'}
fixed={'MAX_PETBATTLE_TEAM':2,'MAX_PETBATTLE_SLOTS':3,'MAX_PETBATTLE_ABILITIES':3,'MAX_DECLINED_NAME_CASES':5,'BATTLEPET_MAX_LEVEL':25,'PETBATTLE_TEAM_1':0,'PETBATTLE_TEAM_2':1,'PETBATTLE_PVE_TEAM_ID':1,'PETBATTLE_NULL_ID':0,'PETBATTLE_NULL_SLOT':-1,'INTERACTION_DISTANCE':5,'BATTLEPET_SPECIES_FLAG_COMPANION':128}
for n in sorted(labels-set(fixed)):fixed[n]=len(fixed)+100
fixed['PETBATTLE_REQUEST_OK']=255
(out/'Labels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n');(out/'Selected.inc').write_text(text);(out/'cases.json').write_text(json.dumps([e['key']for e in rows]));print('Prepared complete journal/duel methods;initial-life/front,XP flag,queue mean and old-removal tail are explicit source projections;actual native paths separate.')
