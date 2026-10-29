#include "GridDefines.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

// Native predicate/coordinate evidence. This is not a forced Map insertion
// failure or a substitute for subtype owner/slot cleanup verification.
int main(int argc,char**)
{
    if(argc!=2)return 1;
    float low=-(MAP_HALFSIZE-0.5f),high=MAP_HALFSIZE-0.5f;
    auto first=Trinity::ComputeCellCoord(low,low),last=Trinity::ComputeCellCoord(high,high);
    assert(first.IsCoordValid()&&last.IsCoordValid());
    assert(first.x_coord==0&&first.y_coord==0);
    assert(last.x_coord==TOTAL_NUMBER_OF_CELLS_PER_MAP-1&&last.y_coord==TOTAL_NUMBER_OF_CELLS_PER_MAP-1);
    // Each cell component is the monotone positive-scale affine conversion in
    // Trinity::Compute. The inclusive native coordinate endpoints bound it.
    uint32 previous=0;
    for(uint32 n=0;n<=1000000;++n)
    {
        float value=float(double(low)+(double(high)-low)*n/1000000.0);
        assert(Trinity::IsValidMapCoord(value,value));
        auto cell=Trinity::ComputeCellCoord(value,value);
        assert(cell.IsCoordValid()&&cell.x_coord>=previous);previous=cell.x_coord;
    }
    for(float invalid:{std::nextafter(low,-std::numeric_limits<float>::infinity()),std::nextafter(high,std::numeric_limits<float>::infinity()),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
        assert(!Trinity::IsValidMapCoord(invalid));
    auto actual=Trinity::ComputeCellCoord(1815.32f,2291.55f);assert(actual.IsCoordValid());
    std::cout<<"PASS native coordinate bounds map to inclusive cells0.."<<last.x_coord<<";1000001 interior samples monotone/valid; adjacent out-of-bounds/nonfinite positions rejected before native Creature::Create can succeed. Not a simulated insertion failure or owner cleanup test.\n";
}
