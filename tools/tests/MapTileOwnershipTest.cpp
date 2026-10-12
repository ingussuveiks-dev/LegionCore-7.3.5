#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) ((void)0)
constexpr int MAX_NUMBER_OF_GRIDS=64;
void Check(bool ok, char const* reason) { if(!ok){std::cerr<<reason<<'\n';std::exit(1);} }
struct GridCoord {int x,y; GridCoord(int a,int b):x(a),y(b){} };
struct GridMap {
 enum class LoadResult {Success,InvalidFile};
 static int alive;
 GridMap(){++alive;} ~GridMap(){--alive;}
 void unloadData(){}
 LoadResult loadData(char const*){return LoadResult::Success;}
};
int GridMap::alive=0;
struct Collision {
 std::map<std::tuple<int,int,int>,int> tiles;
 int loads=0,unloads=0;
 void loadMap(int id,int x,int y){++loads;++tiles[{id,x,y}];}
 void unloadMap(int id,int x,int y){++unloads;Check(tiles.erase({id,x,y})==1,"Duplicate collision tile unload");}
} vmap,mmap;
namespace VMAP {struct VMapFactory{static Collision* createOrGetVMapManager(){return &vmap;}};}
namespace MMAP {struct MMapFactory{static Collision* createOrGetMMapManager(){return &mmap;}};}
struct World{std::string GetDataPath(){return "";}} world;
auto sWorld=&world;
struct Scripts{template<class...T>void OnLoadGridMap(T...){} template<class...T>void OnUnloadGridMap(T...){} } scripts;
auto sScriptMgr=&scripts;
namespace Trinity {template<class...T>std::string StringFormat(char const*,T...){return "fixture.map";}}
struct Map {
 Map* m_parentMap;
 int i_InstanceId;
 GridMap* GridMaps[64][64]{};
 Map(int instance,Map* parent=nullptr):m_parentMap(parent?parent:this),i_InstanceId(instance){}
 int GetId(){return 1220;}
 void LoadMap(int,int,bool=false);
 void LoadMapImpl(Map*,int,int,bool);
 void UnloadMapImpl(Map*,int,int);
 void LoadMapAndVMap(int,int);
 void UnloadGridTiles(int,int);
 void EnsureGridCreated_i(GridCoord p){if(!GridMaps[63-p.x][63-p.y])LoadMapAndVMap(63-p.x,63-p.y);}
 void LoadVMap(int x,int y){vmap.loadMap(GetId(),x,y);}
 void LoadMMap(int x,int y){mmap.loadMap(GetId(),x,y);}
};
struct MapInstanced:Map {
 int refs=0;
 MapInstanced():Map(0){}
 void AddGridMapReference(GridCoord){++refs;}
 void RemoveGridMapReference(GridCoord){Check(refs>0,"Reference underflow");--refs;}
};
#include "MapTileHandlers.inc"
int main(){
 for(int childId:{0,7502,123}) {
  MapInstanced base;
  Map child(childId,&base),other(8445,&base);
  child.LoadMapAndVMap(33,23);
  Check(child.GridMaps[33][23]==base.GridMaps[33][23],"Zone/instance must borrow the base terrain, including ID zero");
  other.LoadMapAndVMap(33,23);
  Check(base.refs==2&&GridMap::alive==1,"Shared terrain must have two borrowers and one allocation");
  Check(vmap.tiles[{1220,33,23}]==1&&mmap.tiles[{1220,33,23}]==1,"Collision must load once");
  // The standalone terrain-unload path must release a borrower only.
  child.UnloadMapImpl(&child,33,23);
  Check(base.refs==1&&GridMap::alive==1&&vmap.tiles.size()==1,"Child terrain cleanup released shared data");
  child.LoadMapAndVMap(33,23);
  // Shutdown grid path: neither borrower may release collision or delete terrain.
  child.UnloadGridTiles(30,40);
  other.UnloadGridTiles(30,40);
  Check(base.refs==0&&GridMap::alive==1&&vmap.tiles.size()==1,"Child grid cleanup released shared data");
  base.UnloadGridTiles(30,40);
  Check(GridMap::alive==0&&vmap.tiles.empty()&&mmap.tiles.empty(),"Base cleanup leaked terrain/collision");
  // Reload after cleanup must establish fresh references and release exactly once.
  child.LoadMapAndVMap(33,23);
  child.UnloadGridTiles(30,40);
  base.UnloadGridTiles(30,40);
 }
 Check(vmap.loads==6&&vmap.unloads==6&&mmap.loads==6&&mmap.unloads==6,"Unbalanced collision lifecycle");
 std::cout<<"PASS: production terrain ownership paths share ID-zero/nonzero zone and instance tiles; two borrowers, cleanup and reload release collision exactly once.\n";
}
