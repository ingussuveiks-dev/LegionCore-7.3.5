// Matching7.3.5 DB2 references to GameObject template IDs; no DB or client save.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 1;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<GameObjectsEntry> objects("GameObjects.db2",GameobjectsLoadInfo::Instance());
 if(!effects.Load(argv[1],0)||!objects.Load(argv[1],0))return 2;
 for(auto e:effects)if(e->Effect==50||e->Effect==76||e->Effect==104||e->Effect==171)std::cout<<"SUMMON\t"<<e->EffectMiscValue[0]<<'\t'<<e->SpellID<<'\t'<<e->ID<<'\t'<<e->Effect<<'\t'<<e->DifficultyID<<'\n';
 for(auto g:objects)std::cout<<"GAMEOBJECT\t"<<g->ID<<'\t'<<g->OwnerID<<'\t'<<uint32(g->TypeID)<<'\n';
 std::cout<<"EOF\n";return 0;}
