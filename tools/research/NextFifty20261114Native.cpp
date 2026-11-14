#include <unordered_set>
#include "SpellPackets.h"
#include "GameObject.h"
#include "Battlefield.h"
#include "MiscPackets.h"
#include "ItemPackets.h"
#include "MovementTypedefs.h"
#include "CommandFloatArgument.h"
#include <iostream>
#include <limits>
#include <type_traits>
int main(){
 static_assert(std::is_same<decltype(WorldPackets::Spells::PlaySpellVisual{}.SpellVisualID),int32>::value,"Native visual signed ID");
 static_assert(std::is_same<decltype(WorldPackets::Spells::PlayOrphanSpellVisual{}.SpellVisualID),int32>::value,"Native orphan signed ID");
 static_assert(std::is_same<decltype(&Object::SetByteValue),void(Object::*)(uint16,uint8,uint8)>::value,"Native byte field/offset/value widths");
 double const oversized=double(1e20f)*1e20f;if(oversized<=std::numeric_limits<float>::max())return 5;
 static_assert(std::is_same<decltype(&Battlefield::SetTimer),void(Battlefield::*)(uint32)>::value,"Native battlefield timer width");
 static_assert(IN_MILLISECONDS==1000&&CURRENT_EXPANSION==6,"Native time/Legion expansion contract");
 static_assert(std::is_same<std::underlying_type<InventoryResult>::type,uint8>::value,"Native equipment enum byte");
 static_assert(SELL_ERR_INTERNAL_BAG_ERROR==9&&BUY_ERR_REPUTATION_REQUIRE==12,"Native non-fixed result enum bounds0..15");
 static_assert(std::is_same<decltype(WorldPackets::Misc::StreamingMovie{}.MovieIDs),std::vector<int16>>::value,"Native streaming IDs are signed16");
 WorldPackets::Misc::StreamingMovie stream;stream.MovieIDs.push_back(std::numeric_limits<int16>::min());auto movie=stream.Write();
 if(movie->size()!=6||movie->read<uint32>(0)!=1||movie->read<int16>(4)!=std::numeric_limits<int16>::min())return 6;
 WorldPackets::Item::SellResponse sell;sell.Reason=SellResult(15);auto sold=sell.Write();if(sold->read<uint8>(sold->size()-1)!=15)return 7;
 WorldPackets::Item::BuyFailed buy;buy.Reason=BuyResult(15);auto bought=buy.Write();if(bought->read<uint8>(bought->size()-1)!=15)return 8;
 float nativeApex=-Movement::computeFallElevation(1e20f/Movement::gravity,false,-1e20f);if(!std::isfinite(nativeApex)||std::isfinite(nativeApex*8.f))return 9;
 float normalApex=-Movement::computeFallElevation(5.f/Movement::gravity,false,-5.f);if(!std::isfinite(normalApex*8.f))return 10;
 double d=7;if(Trinity::ParseCommandDouble("nan",d)||d!=7)return 1;
 if(!Trinity::ParseCommandDouble("2147483647.9",d)||std::trunc(d)!=double(std::numeric_limits<int32>::max()))return 2;
 if(!Trinity::ParseCommandDouble("-2147483648.9",d)||std::trunc(d)!=double(std::numeric_limits<int32>::min()))return 3;
 if(Trinity::ParseCommandDouble("1x",d))return 4;
 std::cout<<"PASS native signed visual packet ID fields;actual finite double parser preserves fractional native integer boundaries and rejected receiver;no DB/world/character mutation\n";return 0;
}
