struct Creature;struct Player;struct Unit;struct GameObject;
struct Battleground{void EventPlayerUsedGO(Player*,GameObject*){}};
namespace WorldPackets::GameObject {struct PageText{template<class T>explicit PageText(T const&){}int Write(){return 0;}};}
struct QuestStore{void* GetQuestTemplate(uint32){return nullptr;}} questStore;
inline auto sQuestDataStore=&questStore;
struct ScriptMgr{bool intercept=false;bool OnGossipHello(Player*,GameObject*){return intercept;}} scriptMgr;
inline auto sScriptMgr=&scriptMgr;
namespace GameTime {inline uint64 GetGameTime(){return 0;}}
inline int sGameObjectScripts=0,sEventScripts=0;
