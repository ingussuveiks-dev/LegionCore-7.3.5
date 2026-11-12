from pathlib import Path
import re,sys
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);require_fixed();e=next(e for e in PLAN['changes']if e['key']=='SinestraIndomitableIsolation');source=e['edits'][0]['new'];assert source.count('GetCaster()->CastSpell(GetTarget(), SPELL_INDOMITABLE_DMG, true);')==1
source=source.replace('GetCaster()->CastSpell(GetTarget(), SPELL_INDOMITABLE_DMG, true);','Observe::CastSpell(GetCaster()->ToCreature(), GetTarget(), SPELL_INDOMITABLE_DMG, true);')
(out/'TenTenFirstNative.inc').write_text('namespace TenTenFirst{enum{SPELL_INDOMITABLE_DMG=90045};'+source+';}\n',encoding='utf8')
print('Prepared actual AuraScript class with a controlled child-cast endpoint;all state/native getters/periodic registration retained')
second=[e for e in PLAN['changes']if e['batch']==2]
if second:
 assert len(second)==10
 protection=next(e for e in second if e['case']=='absorb')['edits'][0]['new']
 assert protection.count('target->CastSpell(target,')==2
 protection=protection.replace('target->CastSpell(target,','ObserveProtection(target, target,')
 metadata='struct SecondBinding{uint32 spell;char const*name;bool scaling;};static SecondBinding secondBindings[]={'+','.join('{'+str(spell)+',"'+e['binding']+'",'+('true'if e['case']=='scaling'else'false')+'}'for e in second if 'binding'in e for spell in e['spells'])+'};\n'
 commands='static char const*secondCommands[]={'+','.join('"'+command[0]+' 123"'for command in second[0]['commands'])+'};\n'
 (out/'TenTenSecondNative.inc').write_text(protection+';\n'+metadata+commands,encoding='utf8')
 print('Prepared actual protection AuraScript with observed child casts and native binding/command metadata for batch2')
third=[e for e in PLAN['changes']if e['batch']==3]
if third:
 assert len(third)==10
 metadata='struct ThirdBinding{char const*kind;char const*name;};static ThirdBinding thirdBindings[]={'+','.join('{"'+e['binding_kind']+'","'+e['binding']+'"}'for e in third)+'};\n'
 e=next(e for e in third if e['case']=='shatter');source=(ROOT/e['paths'][0]).read_text(encoding='utf-8-sig');start=source.index('class spell_eonar_shatter :');end=source.index('\n};',start)+3;body=source[start:end]
 assert body.count('caster->CastSpell(caster,')==4
 body=body.replace('caster->CastSpell(caster,','ObserveProtection(caster, caster,')
 definitions=source+'\n'+(ROOT/'src/server/scripts/Legion/AntorusTheBurningThrone/antorus.h').read_text(encoding='utf-8-sig')
 labels=['NPC_CRYSTAL_TARGETED','NPC_CRYSTAL_BURNING','NPC_CRYSTAL_FOUL_STEPS','NPC_CRYSTAL_ARCANE_SINGULARITY','SPELL_TARGETED','SPELL_BURNING_EMBERS','SPELL_FOUL_STEPS','SPELL_ARCANE_SINGULARITY']
 enums='enum{ACTION_2=2,'+','.join(label+'='+re.search(r'\b'+label+r'\s*=\s*(\d+)',definitions)[1]for label in labels)+'};\n'
 (out/'TenTenThirdNative.inc').write_text(metadata+'namespace ThirdNative{'+enums+body+'}\n',encoding='utf8')
 print('Prepared third10 exact binding metadata and actual Eonar AuraScript/native constants;child cast endpoint observed')
fourth=[e for e in PLAN['changes']if e['batch']==4]
if fourth:
 assert len(fourth)==10
 bindings=[]
 for e in fourth:
  if e['case']=='scarab':bindings += [('instance','instance_temple_of_ahnqiraj'),('instance','instance_ruins_of_ahnqiraj')]
  else:bindings.append((e['binding_kind'],e['binding']))
 text='struct FourthBinding{char const*kind;char const*name;};static FourthBinding fourthBindings[]={'+','.join('{"'+kind+'","'+name+'"}'for kind,name in bindings)+'};\n'
 generic=(ROOT/'src/server/scripts/Spells/spell_generic.cpp').read_text(encoding='utf-8-sig')
 labels=['SPELL_COPY_WEAPON_AURA','SPELL_COPY_WEAPON_2_AURA','SPELL_COPY_WEAPON_3_AURA','SPELL_COPY_OFFHAND_AURA','SPELL_COPY_OFFHAND_2_AURA','SPELL_COPY_RANGED_AURA']
 text+='namespace FourthNative{enum{NPC_AVATARA_MAIDEN=117264,'+','.join(label+'='+re.search(r'\b'+label+r'\s*=\s*(\d+)',generic)[1]for label in labels)+'};\n'
 e=next(e for e in fourth if e['case']=='clone_weapon');text+=e['edits'][e.get('class_edit_index',0)]['new']+';\n'
 e=next(e for e in fourth if e['case']=='lilet');body=e['edits'][0]['new'];assert body.count('Talk(talk, owner->GetGUID());')==1;text+=body.replace('Talk(talk, owner->GetGUID());','ObserveFourthSpeech(me, talk, owner->GetGUID());')+';\n'
 e=next(e for e in fourth if e['case']=='avatar');registration=e['edits'][0]['new']
 text+='struct MaidenEndpoint{Creature*actor;Creature*GetCreature(uint32){return Observe::queryPresent?actor:nullptr;}};struct InstanceEndpoint{void SendEncounterUnit(uint32,Creature*actor){Observe::Mark(actor,2);}};\n'
 text+='struct AvatarFrame:ScriptedAI{explicit AvatarFrame(Creature*c):ScriptedAI(c),summons{c}{};std::shared_ptr<uint8> _delayedAILifetimeGuard=std::make_shared<uint8>(0);MaidenEndpoint summons;InstanceEndpoint endpoint;InstanceEndpoint*instance=&endpoint;void DoZoneInCombat(Creature*c){Observe::Mark(c,1);}void Queue(uint32){'+registration+'}};\n}\n'
 (out/'TenTenFourthNative.inc').write_text(text,encoding='utf8')
 print('Prepared actual native clone AuraScript/Lilet methods and full Avatar registration;spell/speech/instance endpoints are explicitly observed')
fifth=[e for e in PLAN['changes']if e['batch']==5]
if fifth:
 from prepare_ten_ten_20261111_fifth import block
 text='struct FifthBinding{char const*kind;char const*name;};static FifthBinding fifthBindings[]={'+','.join('{"'+e['binding_kind']+'","'+e['binding']+'"}'for e in fifth)+'};\nnamespace FifthNative{\n'
 e=fifth[0];body=e['edits'][1]['new'];assert body.count('caster->CastSpell(target, spellId, true);')==1;text+=body.replace('caster->CastSpell(target, spellId, true);','ObserveProtection(caster, target, spellId, true);')+';\n'
 e=fifth[1];body=e['edits'][0]['new'];body=block(body,r'if \(auto player = summoner->ToPlayer\(\)\)');text+='void CopyFigment(Unit*summoner,Creature*creature){'+body+'}\n'
 e=fifth[4];body=e['edits'][0]['new'].replace('player->GroupEventHappens(QUEST_10004, me);','ObserveFifthCredit(player, QUEST_10004, me);');text+='enum{QUEST_10004=10004};struct Salsa:ScriptedAI{using ScriptedAI::ScriptedAI;void EnterEvadeMode()override{++fifthEvades;}'+body+'};\n'
 e=fifth[9];body=e['edits'][0]['new'].replace('GetPlayerListInGrid(targets, me, 70.0f);','ObserveFifthPlayers(targets);').replace('me->GetMotionMaster()->MoveFollow((*itr), PET_FOLLOW_DIST, (*itr)->GetFollowAngle());','ObserveFifthFollow(me, *itr);').replace('me->CastSpell((*itr), 212943);','ObserveProtection(me, *itr, 212943, false);').replace('DoCast(target, 212948, true);','ObserveProtection(me, target, 212948, true);');text+='struct Orb:ScriptedAI{using ScriptedAI::ScriptedAI;ObjectGuid targetMove;bool moveComplete=true,CheckComplete=false;uint32 moveTimer=0,checkTimer=2000;'+body+'};\n}\n'
 (out/'TenTenFifthNative.inc').write_text(text,encoding='utf8')
 print('Prepared fifth actual native hook, paired fields, affecting-player andstun-state scopes;cast/group/movement/query endpoints observed')
