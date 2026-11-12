from pathlib import Path
import json,re,subprocess,sys
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[e for e in PLAN['changes']if e['batch']==2];assert len(rows)==10
def source(path):
 if mode=='baseline':return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['batch_baselines']['2']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
 return(ROOT/path).read_text(encoding='utf-8-sig')
pb=source('src/server/game/PetBattle/PetBattle.cpp');ps=source('src/server/game/PetBattle/PetBattleSystem.cpp');ph=source('src/server/game/Handlers/BattlePetHandler.cpp');pe=source('src/server/game/PetBattle/PetBattleAbilityEffect.cpp')
text='\n'.join([block(ph,r'void WorldSession::HandlePetBattleRequestUpdate\('),block(ps,r'eBattlePetRequests PetBattleSystem::CanPlayerEnterInPetBattle\('),block(pb,r'void PetBattle::Finish\('),block(pb,r'bool PetBattleTeam::CanCastAny\(')])
text+='\n'.join(block(pe,r'bool PetBattleAbilityEffect::'+name+r'\(')for name in ['HandleHealPetType','HandleDamageWithBonus','HandleDamageStateBonus','HandleRampingDamage','HandleRemoveAura','HandleSplittedDamage'])
text+='\n'+block(ps,r'PetBattleRequest\* PetBattleSystem::CreateRequest\(')+'\n'+block(ps,r'void PetBattleSystem::RemoveRequest\(')
labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b|\bBATTLEPET_STATE_\w+',text))|{'PETBATTLE_STATUS_RUNNING','BATTLEPET_STATE_Is_Dead','PETBATTLE_TYPE_PVP_DUEL','BATTLEPET_STATE_Stat_Accuracy','BATTLEPET_STATE_Stat_Dodge','BATTLEPET_STATE_Passive_Aquatic','PETBATTLE_EVENT_FLAG_PERIODIC','PETBATTLE_TEAM_CATCH_FLAG_ENABLE_TRAP'}
fixed={'MAX_PETBATTLE_TEAM':2,'MAX_PETBATTLE_SLOTS':3,'BATTLEPET_MAX_LEVEL':25,'PETBATTLE_TEAM_1':0,'PETBATTLE_TEAM_2':1,'PETBATTLE_PVE_TEAM_ID':1,'PETBATTLE_NULL_ID':0,'PETBATTLE_NULL_SLOT':-1,'INTERACTION_DISTANCE':5}
for n in sorted(labels-set(fixed)):fixed[n]=len(fixed)+100
fixed['PETBATTLE_REQUEST_OK']=255;fixed['PETBATTLE_EVENT_FLAG_PERIODIC']=2
(out/'Labels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n');(out/'Selected.inc').write_text(text);(out/'cases.json').write_text(json.dumps([e['key']for e in rows]));print('Prepared complete second10 production methods;external endpoints controlled,native catalogue/linked/client separate.')
