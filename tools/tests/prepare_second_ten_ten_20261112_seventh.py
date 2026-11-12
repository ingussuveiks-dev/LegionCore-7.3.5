"""Complete native pet methods with controlled DB2/event/session endpoints."""
from pathlib import Path
import sys,re,json,subprocess
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[r for r in PLAN['changes']if r['batch']==7];assert len(rows)==10
def src(path):
 if mode=='baseline':return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['batch_baselines']['7']+':'+path]).decode('utf-8-sig',errors='surrogateescape').replace('\r\n','\n')
 return(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape')
pb=src('src/server/game/PetBattle/PetBattle.cpp');pe=src('src/server/game/PetBattle/PetBattleAbilityEffect.cpp');h=src('src/server/game/PetBattle/PetBattle.h');eh=src('src/server/game/PetBattle/PetBattleAbilityEffect.h')
enums=[]
for text in [h,eh]:
 for m in re.finditer(r'(?m)^enum(?: class)? (\w+)',text):enums.append(block(text[m.start():],r'enum(?: class)? '+m[1]+r'\b')+';')
defines=[l for l in h.splitlines()if l.startswith('#define ')];(out/'PetEnums.inc').write_text('\n'.join(defines+enums)+'\n')
classes='\n'.join(block(eh,r'struct '+cls+r'\b')+';'for cls in ['PetBattleAbilityTurn','PetBattleAbilityEffect']);(out/'PetEffectClass.inc').write_text(classes+'\n')
table=pe[pe.index('struct PetBattleAbilityEffectHandler'):pe.index('uint32 PetBattleAbilityEffect::GetActiveOpponent()')];(out/'PetHandlers.inc').write_text(table)
methods={'GetActiveOpponent':'uint32','GetPetType':'int32','GetState':'int32','GetHealth':'int32','GetMaxHealth':'int32','GetSpeed':'int32','CalculateHit':'int32','CalculateDamage':'int32','CalculateHeal':'int32','SetState':'bool','SetHealth':'bool','Damage':'bool','Heal':'bool','ModState':'void','AddAura':'bool','SelectTargets':'void','Execute':'bool','GetAura':'PetBattleAura*','HandleExtraAttackIfMoreFaster':'bool','HandleExtraAttackIfLessFaster':'bool','HandleModState':'bool','HandleResetAuraDuration':'bool','HandleHealPercent':'bool','HandlePositiveAura':'bool','HandleDamage':'bool','HandleRemoveAura':'bool'}
methods.update(HandlePetTypeAura='bool',ConsumeBlockCharge='bool',HandleDamageToggleAura='bool',HandleHealToggleAura='bool',HandleHealStateToggleAura='bool',HandleRedirectionAura='bool',HandleStateDamage='bool',HandleDamagePercent='bool',HandleHealCasterPercentNotState='bool',HandleDamagePercentTaken='bool',HandleDummy='bool')
parts=[]
for name,ret in methods.items():parts.append(block(pe,re.escape(ret)+r' PetBattleAbilityEffect::'+name+r'\('))
parts+= [block(pe,r'bool PetBattleAbilityEffect::AddTarget\(uint32 target\)'),block(pe,r'bool PetBattleAbilityEffect::AddTarget\(PetBattleAbilityImplicitTarget')]
implemented=set(methods)
for name in re.findall(r'bool (Handle\w+)\(\);',classes):
 if name not in implemented:parts.append('bool PetBattleAbilityEffect::'+name+'(){return false;}')
parts+=[block(pb,r'int32 BattlePetInstance::GetMaxHealth\('),block(pb,r'PetBattleCastResult PetBattle::Cast\('),block(pb,r'bool PetBattle::AddAura\('),block(pb,r'void PetBattle::SetPetState\('),block(pb,r'void PetBattle::Kill\(')]
parts.append(block(pb,r'static void UpdatePetBattleAuraMaxHealth\('))
parts += [block(pb,r'void PetBattleAura::'+name+r'\(')for name in ['Apply','Remove','Process','Expire']]
(out/'Selected.inc').write_text('\n'.join(parts)+'\n');(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));(out/'fixture-mode.json').write_text(json.dumps({'mode':mode,'scope':'Complete selected production effect/core/aura methods and complete production handler table; non-selected handlers and external DB2/session/world endpoints controlled.'}));print('Prepared complete selected pet core/effect/aura methods, exact native enums and handler table; native full-runtime catalogue and client scopes remain separate.')

ps=src('src/server/game/PetBattle/PetBattleSystem.cpp');ad=block(ps,r'eBattlePetRequests PetBattleSystem::CanPlayerEnterInPetBattle\(');(out/'Admission.inc').write_text(ad)
labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b|\bBATTLEPET_STATE_\w+',ad))|{'PETBATTLE_STATUS_RUNNING','BATTLEPET_STATE_Is_Dead','PETBATTLE_TYPE_PVP_DUEL','CRITERIA_TYPE_BATTLEPET_LEVEL_UP','CRITERIA_TYPE_BATTLEPET_WIN','PETBATTLE_TEAM_CATCH_FLAG_ENABLE_TRAP','PETBATTLE_TYPE_PVE','PVE_PETBATTLE_TRAINER'}
fixed={'MAX_PETBATTLE_TEAM':2,'MAX_PETBATTLE_SLOTS':3,'MAX_PETBATTLE_ABILITIES':3,'PETBATTLE_TEAM_1':0,'PETBATTLE_TEAM_2':1,'PETBATTLE_NULL_SLOT':-1,'INTERACTION_DISTANCE':5}
for n in sorted(labels-set(fixed)):fixed[n]=len(fixed)+100
fixed['PETBATTLE_REQUEST_OK']=255
(out/'Labels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n')
