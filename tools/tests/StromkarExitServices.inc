// Controlled storage, spell service and aura application endpoints around the
// extracted production Creature addon selector/loader and SmartAI dispatcher.
struct SpellInfo{bool IsChanneled()const{return false;}};
struct ExitSpellMgr{SpellInfo info;SpellInfo const*GetSpellInfo(uint32 id){return id==202925||id==204345||id==29266?&info:nullptr;}}exitSpellMgr;auto*sSpellMgr=&exitSpellMgr;
struct CreatureAddon{uint32 path_id=0,mount=0,bytes1=0,bytes2=0,emote=0;std::vector<uint32>auras;};
struct ExitTemplate{uint32 Entry=0;};
struct ExitObjectMgr{std::map<uint32,CreatureAddon>spawn,templates;CreatureAddon const*GetCreatureAddon(uint32 id){auto i=spawn.find(id);return i==spawn.end()?nullptr:&i->second;}CreatureAddon const*GetCreatureTemplateAddon(uint32 id){auto i=templates.find(id);return i==templates.end()?nullptr:&i->second;}}exitObjectMgr;auto*sObjectMgr=&exitObjectMgr;
constexpr uint32 UNIT_FIELD_BYTES_1=0,UNIT_FIELD_BYTES_2=0,UNIT_BYTES_2_OFFSET_SHEATH_STATE=0,UNIT_BYTES_2_OFFSET_PET_FLAGS=0,UNIT_BYTES_2_OFFSET_SHAPESHIFT_FORM=0,UNIT_FIELD_EMOTE_STATE=0;
