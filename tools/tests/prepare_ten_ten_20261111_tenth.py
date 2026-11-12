"""Final scopes: complete methods/classes plus explicit history recording/action projections."""
import json,re,sqlite3
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block

def prepare(out,mode):
 rows=[e for e in PLAN['changes']if e['batch']==10];assert len(rows)==10;require_fixed([e['key']for e in rows]);out.mkdir(parents=True,exist_ok=True)
 def source(path):
  s=(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape')
  if mode=='baseline':
   for e in reversed(rows):
    for ed in reversed(e['edits']):
     if ed['path']==path:assert s.count(ed['new'])==1;s=s.replace(ed['new'],ed['old'],1)
  return s
 text='namespace History{\n'
 e=rows[0];h=source(e['paths'][0]);cpp=source(e['paths'][1]);wide='uint64 GetDamageTakenInPastSecs'in h
 text+='using Amount='+('uint64'if wide else'uint32')+';struct Unit{std::array<Amount,120>npcDamageTaken_{},playerDamageTaken_{};'+''.join(re.search(r'uint(?:32|64) Get'+n+r'InPastSecs\([^;]+;',h)[0]for n in('NpcDamageTaken','PlayerDamageTaken','DamageTaken'))+'};\n'
 text+='\n'.join(block(cpp,r'uint(?:32|64) Unit::Get'+n+r'InPastSecs\(')for n in('NpcDamageTaken','PlayerDamageTaken','DamageTaken'))
 text+='\nstruct Clean{uint32 absorbed_damage=0;};void Record(Unit*victim,uint32 damage,Clean*cleanDamage,bool player){if(!player)'+e['edits'][5]['old'if mode=='baseline'else'new']+'else '+e['edits'][6]['old'if mode=='baseline'else'new']+'}\n}\n'
 text+='namespace FinalSpells{enum{SPELLVALUE_BASE_POINT0=0,ScatterTheShadows='+re.search(r'\bScatterTheShadows\s*=\s*(\d+)',(ROOT/'src/server/scripts/Spells/spell_paladin.cpp').read_text())[1]+',SPELL_WARRIOR_COMMANDING_SHOUT_AURA='+re.search(r'\bSPELL_WARRIOR_COMMANDING_SHOUT_AURA\s*=\s*(\d+)',(ROOT/'src/server/scripts/Spells/spell_warrior.cpp').read_text())[1]+'};\n'
 for e in rows[1:4]:text+=e['edits'][0]['old'if mode=='baseline'else'new']+';\n'
 text+='}\nnamespace CoreHealth{\n'
 a=source('src/server/game/Spells/Auras/SpellAuraEffects.cpp');st=source('src/server/game/Entities/Unit/StatSystem.cpp')
 for name in ('HandleAuraModIncreaseHealth','HandleAuraModIncreaseHealthOrPercent','HandleAuraModIncreaseHealthPercent'):text+=block(a,r'void AuraEffect::'+name+r'\(')+'\n'
 text+=block(st,r'void Player::UpdateMaxHealth\(')+'\n}\n'
 e=rows[5];text+='namespace SmartFixture{void Script::Run(Event&e,Unit*unit){switch(e.action.type){'+e['edits'][0]['old'if mode=='baseline'else'new']+'default:break;}}}\n'
 connection=sqlite3.connect(':memory:');connection.execute('CREATE TABLE spell_script_names(spell_id INTEGER,ScriptName TEXT)');connection.executemany('INSERT INTO spell_script_names VALUES(?,?)',[(69893,'spell_copy_melee_weapon'),(69893,'spell_gen_clone_weapon_aura'),(41054,'spell_gen_clone_weapon_aura')]);sql=source(rows[4]['paths'][0]);connection.executescript(sql);connection.executescript(sql);count=connection.execute('SELECT COUNT(*) FROM spell_script_names WHERE spell_id=69893').fetchone()[0];text+='static constexpr int CloneHandlersAfterActualSql='+str(count)+';\n'
 pb=source('src/server/game/PetBattle/PetBattle.cpp');pe=source('src/server/game/PetBattle/PetBattleAbilityEffect.cpp')
 text+='namespace PetFinal{\n'+''.join(block(pb,r'void PetBattleAura::'+name+r'\(')+'\n'for name in('Apply','Remove'))+block(pb,r'bool PetBattle::AddAura\(')+'\n'+block(pe,r'bool PetBattleAbilityEffect::HandleWeatherAura\(')+'\n'+block(pb,r'int32 BattlePetInstance::GetSpeed\(')+'\n}\n'
 (out/'Selected.inc').write_text(text,encoding='utf8');(out/'cases.json').write_text(json.dumps([e['key']for e in rows]));print('Extracted final production methods/classes,actual SQL shadow,explicit history-record/action cases;external endpoints controlled')
