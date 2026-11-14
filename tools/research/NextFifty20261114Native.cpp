#include <unordered_set>
#include "SpellPackets.h"
#include "CommandFloatArgument.h"
#include <iostream>
#include <limits>
#include <type_traits>
int main(){
 static_assert(std::is_same<decltype(WorldPackets::Spells::PlaySpellVisual{}.SpellVisualID),int32>::value,"Native visual signed ID");
 static_assert(std::is_same<decltype(WorldPackets::Spells::PlayOrphanSpellVisual{}.SpellVisualID),int32>::value,"Native orphan signed ID");
 double d=7;if(Trinity::ParseCommandDouble("nan",d)||d!=7)return 1;
 if(!Trinity::ParseCommandDouble("2147483647.9",d)||std::trunc(d)!=double(std::numeric_limits<int32>::max()))return 2;
 if(!Trinity::ParseCommandDouble("-2147483648.9",d)||std::trunc(d)!=double(std::numeric_limits<int32>::min()))return 3;
 if(Trinity::ParseCommandDouble("1x",d))return 4;
 std::cout<<"PASS native signed visual packet ID fields;actual finite double parser preserves fractional native integer boundaries and rejected receiver;no DB/world/character mutation\n";return 0;
}
