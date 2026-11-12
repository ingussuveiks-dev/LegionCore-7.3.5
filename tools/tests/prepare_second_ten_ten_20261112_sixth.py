"""Complete native pet methods with controlled DB2/event/session endpoints."""
from pathlib import Path
import sys,re,json,subprocess
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[r for r in PLAN['changes']if r['batch']==6];assert len(rows)==10
def src(path):
 if mode=='baseline':return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['batch_baselines']['6']+':'+path]).decode('utf-8-sig',errors='surrogateescape').replace('\r\n','\n')
 return(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape')
pb=src('src/server/game/PetBattle/PetBattle.cpp');pe=src('src/server/game/PetBattle/PetBattleAbilityEffect.cpp');h=src('src/server/game/PetBattle/PetBattle.h');eh=src('src/server/game/PetBattle/PetBattleAbilityEffect.h')
enums=[]
for text in [h,eh]:
 for m in re.finditer(r'(?m)^enum(?: class)? (\w+)',text):enums.append(block(text[m.start():],r'enum(?: class)? '+m[1]+r'\b')+';')
defines=[l for l in h.splitlines()if l.startswith('#define ')];(out/'PetEnums.inc').write_text('\n'.join(defines+enums)+'\n')
classes='\n'.join(block(eh,r'struct '+cls+r'\b')+';'for cls in ['PetBattleAbilityTurn','PetBattleAbilityEffect']);(out/'PetEffectClass.inc').write_text(classes+'\n')
table=pe[pe.index('struct PetBattleAbilityEffectHandler'):pe.index('uint32 PetBattleAbilityEffect::GetActiveOpponent()')];(out/'PetHandlers.inc').write_text(table)
methods={'GetActiveOpponent':'uint32','GetPetType':'int32','GetState':'int32','GetHealth':'int32','GetMaxHealth':'int32','GetSpeed':'int32','CalculateHit':'int32','CalculateDamage':'int32','CalculateHeal':'int32','SetState':'bool','SetHealth':'bool','Damage':'bool','Heal':'bool','ModState':'void','AddAura':'bool','SelectTargets':'void','Execute':'bool','GetAura':'PetBattleAura*','HandleExtraAttackIfMoreFaster':'bool','HandleExtraAttackIfLessFaster':'bool','HandleModState':'bool','HandleResetAuraDuration':'bool','HandleHealPercent':'bool','HandlePositiveAura':'bool','HandleDamage':'bool','HandleRemoveAura':'bool'}
if mode=='fixed':methods.update(HandlePetTypeAura='bool',ConsumeBlockCharge='bool')
parts=[]
for name,ret in methods.items():parts.append(block(pe,re.escape(ret)+r' PetBattleAbilityEffect::'+name+r'\('))
parts+= [block(pe,r'bool PetBattleAbilityEffect::AddTarget\(uint32 target\)'),block(pe,r'bool PetBattleAbilityEffect::AddTarget\(PetBattleAbilityImplicitTarget')]
implemented=set(methods)
for name in re.findall(r'bool (Handle\w+)\(\);',classes):
 if name not in implemented:parts.append('bool PetBattleAbilityEffect::'+name+'(){return false;}')
parts+=[block(pb,r'int32 BattlePetInstance::GetMaxHealth\('),block(pb,r'PetBattleCastResult PetBattle::Cast\('),block(pb,r'bool PetBattle::AddAura\('),block(pb,r'void PetBattle::SetPetState\('),block(pb,r'void PetBattle::Kill\(')]
if mode=='fixed':parts.append(block(pb,r'static void UpdatePetBattleAuraMaxHealth\('))
parts += [block(pb,r'void PetBattleAura::'+name+r'\(')for name in ['Apply','Remove','Process','Expire']]
(out/'Selected.inc').write_text('\n'.join(parts)+'\n');(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));(out/'fixture-mode.json').write_text(json.dumps({'mode':mode,'scope':'Complete selected production effect/core/aura methods and complete production handler table; non-selected handlers and external DB2/session/world endpoints controlled.'}));print('Prepared complete selected pet core/effect/aura methods, exact native enums and handler table; native full-runtime catalogue and client scopes remain separate.')
