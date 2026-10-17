// These GO origins are up to5.12yd below sea level. Search the surface
// water layer too, then apply actual three-dimensional native interaction
// distance and physical surface checks. This does not add navigation data
// or assert that the 2D water mesh represents a submerged swimming path.
#define NAV_INTERACTION_VERTICAL_EXTENT 8
#define NAV_SOURCE_INTERACTION(root,center,point) (NAV_TARGET_INTERACTION(root,center,point)&&surfaces.Check(point,center,false))
#include "StormheimShieldApproach.cpp"
