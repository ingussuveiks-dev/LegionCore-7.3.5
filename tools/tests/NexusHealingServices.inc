// Controlled scenario/achievement services for the production52/44 branches.
using InstanceMap=Map;
constexpr uint32 CONDITION_CRITERIA_TREE=52,CONDITION_SCENARION_STEP=44;
struct CriteriaTree{uint32 id=47107;} scionLeaf;
struct HealingAchievement {bool scions=false;bool IsCompletedCriteriaTree(CriteriaTree const* tree){return tree&&tree->id==47107&&scions;}};
struct HealingScenario {uint32 id=1065,step=0;bool healed=false;HealingAchievement achievement;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}HealingAchievement&GetAchievementMgr(){return achievement;}}healingScenario;
using Scenario=HealingScenario;
struct HealingScenarioManager {bool present=true;HealingScenario*GetScenario(uint32 instance){return present&&instance==1?&healingScenario:nullptr;}}healingScenarioManager;auto*sScenarioMgr=&healingScenarioManager;
struct HealingAchievementManager {CriteriaTree const*GetCriteriaTree(uint32 id){return id==47107?&scionLeaf:nullptr;}}healingAchievementManager;auto*sAchievementMgr=&healingAchievementManager;
bool acceptCriteria=true;
