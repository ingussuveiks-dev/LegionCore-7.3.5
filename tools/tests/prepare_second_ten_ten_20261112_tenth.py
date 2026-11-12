from pathlib import Path
import sys,re,json,subprocess
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[r for r in PLAN['changes']if r['batch']==10];assert len(rows)==10
def source(path):
 if mode=='baseline':return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['batch_baselines']['10']+':'+path]).decode('utf-8-sig',errors='surrogateescape').replace('\r\n','\n')
 return(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape')
def body(i):return rows[i]['edits'][0]['old'if mode=='baseline'else'new']
def labels(filename,text,extra=None):
 names=set(re.findall(r'\b(?:EVENT|SPELL|SAY|DATA|NPC|PHASE|STAGE|AURA|MOB|TEXT|LFB)_[A-Za-z_0-9]+\b|\b[A-Z][A-Z_0-9]+\b',re.sub(r'//[^\n]*|/\*.*?\*/',' ',text,flags=re.S)))
 fixed={'IN_MILLISECONDS':1000,'IN_PROGRESS':1,'DONE':3,'TO_BE_DECIDED':5,'NOT_STARTED':0,'TYPEID_PLAYER':1,'TYPEID_UNIT':2,'BASE_ATTACK_TIME':2000,'BASE_ATTACK':0,'OFF_ATTACK':1,'EFFECT_0':0,'EFFECT_1':1,'EFFECT_2':2,'EFFECT_3':3,'EFFECT_4':4,'MAX_SPELL_EFFECTS':32,'MAX_PETBATTLE_SLOTS':3,'MAX_PETBATTLE_ABILITIES':3,'CONFIG_PET_BATTLES':1,'PETBATTLE_REQUEST_OK':255,'PETBATTLE_REQUEST_CREATE_FAILED':254}
 if extra:fixed.update(extra)
 for name in sorted(names-set(fixed)-{'NULL','TC_LOG_ERROR'}):fixed[name]=len(fixed)+100
 (out/filename).write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n')
clock='struct TenthClock0:ClockAI{using ClockAI::ClockAI;bool _enraged=false;'+ '\n'.join(block(body(0),r'void '+m+r'\(')for m in ['Reset','UpdateAI'])+'};'
labels('TenthClockLabels.inc',clock);(out/'TenthClockSelected.inc').write_text(clock)
spell=body(1)+';\nstruct TenthClone{Unit*owner=nullptr;Creature*me=nullptr;uint32 entryId=69791;float oldHast=0;'+ '\n'.join(block(body(2),r'void '+m+r'\(')for m in ['RecalcDamage','RecalcHast'])+'};\n'
spell+='struct TenthHarvest{'+('InstanceScript* instance=nullptr;'if mode=='baseline'else'')+block(body(3),r'bool OnTrigger\(').replace(' override','')+'};\n'+body(4)+';\n'+body(6)+';\n'
labels('TenthSpellLabels.inc',spell,{'SoulFragment1':204255,'SoulFragment2':203795,'SoulFragment3':204062});(out/'TenthSpellSelected.inc').write_text(spell)
ticks=body(5);labels('TenthTickLabels.inc',ticks);(out/'TenthTicks.inc').write_text(ticks)
actor=body(7);labels('TenthActorLabels.inc',actor);(out/'TenthActorSelected.inc').write_text(actor+';\n')
ps=source('src/server/game/PetBattle/PetBattleSystem.cpp');bh=source('src/server/game/Handlers/BattlePetHandler.cpp')
packet=block(bh,r'void WorldSession::HandleJoinPetBattleQueue\(');leave=block(ps,r'void PetBattleSystem::LeaveQueue\(');update=block(ps,r'void PetBattleSystem::Update\(')
start=update.index('else if (l_Left->ProposalAnswer == LFBAnswer::LFB_ANSWER_DENY');decline=block(update[start:],r'else if \(').replace('else if','if',1)
helper=block(ps,r'eBattlePetRequests PetBattleSystem::CheckQueueEntry\(')if mode=='fixed'else'eBattlePetRequests PetBattleSystem::CheckQueueEntry(Player*){return PETBATTLE_REQUEST_OK;}'
queue=helper+'\n'+packet+'\n'+leave+'\nvoid PetBattleSystem::Decline(LFBTicket*l_Left,LFBTicket*l_Right,Player*leftPlayer,Player*rightPlayer,std::vector<ObjectGuid>&ticketsToRemove){'+decline+'}\n'
queue=queue.replace('LFBState::','').replace('LFBAnswer::','').replace('LFBUpdateStatus::','')
labels('TenthQueueLabels.inc',queue,{'LFB_ANSWER_PENDING':-1,'LFB_ANSWER_DENY':0,'LFB_ANSWER_AGREE':1,'LFB_STATE_QUEUED':1,'LFB_STATE_PROPOSAL':2,'LFB_STATE_IN_COMBAT':3,'LFB_STATE_FINISHED':4});(out/'TenthQueueSelected.inc').write_text(queue)
(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));(out/'fixture-scope.json').write_text(json.dumps({'mode':mode,'complete':['Kurinnaxx Reset/UpdateAI','ShadowReflection complete AuraScript','Clone RecalcDamage/RecalcHast','BlackHarvest complete OnTrigger','SpiritBomb complete SpellScript','SpellInfo GetMaxTicks','HolyWrath complete SpellScript','Corla complete AI class','Queue packet handler and LeaveQueue; repaired complete CheckQueueEntry'],'projections':['Complete decline branch projected into a harness; actual native Update separately exercises accepted rejection and same-update three-player rematch. Baseline packet counterfactual uses the old complete handler; baseline helper stub is never the source of that failure.'],'controlled':'World/instance/spatial/cast/item/factory/random endpoints; actual HealingAmount math; actual native EventMap/Aura/Spell/queue scopes are separate.'},indent=2));print('Prepared final ten complete declared production scopes and explicitly named queue projections; native gameplay and client scopes remain separate.')
