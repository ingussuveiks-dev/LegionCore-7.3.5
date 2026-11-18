// Matching DB2/Hotfix spell metadata and real registered Spell/Aura validators.
// These are loader checks, not casts, boss combat or a client traversal.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"
struct MechanicSpellRef{uint32 map;char const* source;char const* name;uint32 spell;};
#include "LegionMechanicsRefs.inc"
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);auto modes=sDB2Manager.GetAllMapsDifficultyes();std::set<uint32> spells;unsigned modeRefs=0,missing=0,validated=0;
 for(auto const&row:mechanicRefs){spells.insert(row.spell);auto info=sSpellMgr->GetSpellInfo(row.spell);if(!info){++missing;std::cout<<"MISSING spell="<<row.spell<<" map="<<row.map<<" symbol="<<row.name<<" source="<<row.source<<'\n';continue;}
  for(auto const&mode:modes[row.map]){uint32 mask=0;for(unsigned effect=0;effect<MAX_SPELL_EFFECTS;++effect)if(info->GetEffect(SpellEffIndex(effect),mode.first)->Effect)mask|=uint32(1)<<effect;++modeRefs;if(!mask)std::cout<<"EMPTY spell="<<row.spell<<" mode="<<mode.first<<" source="<<row.source<<'\n';}
 }
 for(auto id:{227977u,239217u}){auto info=sSpellMgr->GetSpellInfo(id);auto text=sSpellStore.LookupEntry(id);if(info&&text){std::cout<<"GLARE spell="<<id<<" name="<<text->Name->Str[LOCALE_enUS]<<" description="<<text->Description->Str[LOCALE_enUS]<<'\n';for(uint32 mode:{2u,8u,23u})for(unsigned i=0;i<3;++i){auto e=info->GetEffect(SpellEffIndex(i),mode);std::cout<<"GLARE_EFFECT spell="<<id<<" mode="<<mode<<" index="<<i<<" effect="<<e->Effect<<" targetA="<<e->TargetA.GetTarget()<<" targetB="<<e->TargetB.GetTarget()<<'\n';}}}
 std::set<std::string> loaders(std::begin(mechanicLoaders),std::end(mechanicLoaders));std::set<std::string> bound;
 auto rows=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names ORDER BY spell_id,ScriptName");
 if(rows)do{auto f=rows->Fetch();auto name=f[1].GetString();if(!loaders.count(name))continue;int32 raw=f[0].GetInt32();uint32 id=uint32(raw<0?-raw:raw);auto info=sSpellMgr->GetSpellInfo(id);Require(info,"Bound dungeon spell absent in native data");auto loader=sScriptMgr->GetSpellScriptLoader(sObjectMgr->GetScriptId(name.c_str()));Require(loader,"Declared dungeon spell loader not actually registered");
  bool valid=true;if(auto script=std::unique_ptr<SpellScript>(loader->GetSpellScript())){script->_Init(&name,id);script->_Register();valid&=script->_Validate(info);for(auto&hook:script->OnObjectAreaTargetSelect)Require(hook.GetAffectedEffectsMask(info),"Registered area-target handler has no authored effect");
   if(name=="spell_coen_blinding_glare"){
    for(auto mode:{DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE}){
     MapInstanced parent(1677,300);InstanceMap map(1677,300,9946000+uint32(mode)+id,mode,&parent);map.CreateInstanceData(nullptr);Position casterPose{-699.75f,2528.56f,332.08f,0};LifecyclePlayer caster(map,401,casterPose);auto facingPose=casterPose;facingPose.m_positionX+=2;facingPose.SetOrientation(float(M_PI));auto awayPose=facingPose;awayPose.SetOrientation(0);LifecyclePlayer facing(map,402,facingPose),away(map,403,awayPose);TriggerCastData trigger;trigger.triggerFlags=TRIGGERED_FULL_MASK;Spell cast(&caster,info,trigger);Require(script->_Load(&cast),"Actual shared glare script load failed");
     uint32 matched=0;for(unsigned effect=0;effect<3;++effect){bool authored=info->GetEffect(SpellEffIndex(effect),uint8(mode))->Effect!=0;std::list<WorldObject*> targets{&facing,&away,nullptr};unsigned handlers=0;for(auto&hook:script->OnObjectAreaTargetSelect)if(hook.IsEffectAffectedForDifficulty(info,uint8(effect),uint8(mode),TARGET_UNIT_SRC_AREA_ENEMY)){hook.Call(script.get(),targets);++handlers;}Check(handlers==(authored?1u:0u),"Shared glare does not dispatch once per real difficulty effect");if(authored){Check(targets.size()==1&&targets.front()==&facing,"Actual shared glare filter lost facing/null/away selection");matched|=1u<<effect;}}
     Check(matched==(id==227977?3u:7u),"Native shared glare effect footprint changed");
    }
   }
  }if(auto script=std::unique_ptr<AuraScript>(loader->GetAuraScript())){script->_Init(&name,id);script->_Register();valid&=script->_Validate(info);}Require(valid,"Actual dungeon spell/aura validation failed");++validated;bound.insert(name);
 }while(rows->NextRow());
 for(auto const&name:loaders)if(!bound.count(name))std::cout<<"UNBOUND loader="<<name<<'\n';
 std::cout<<"CATALOG references="<<std::size(mechanicRefs)<<" uniqueSpells="<<spells.size()<<" nativeModeRefs="<<modeRefs<<" boundLoaderPairs="<<validated<<" declaredLoaders="<<loaders.size()<<" boundLoaders="<<bound.size()<<" missingReferences="<<missing<<'\n';
 if(!missing)std::cout<<"PASS scoped native mechanics metadata/registered validators;combat execution,all source expressions,trash paths,loot and client remain separate\n";
 else std::cout<<"REVIEW missing numeric references require source-use investigation before declaring their mechanics complete\n";
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
