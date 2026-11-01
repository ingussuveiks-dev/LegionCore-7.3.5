# Explicit independent-delta certificate for retained pre-acquisition suites.
# Their production companion handlers still compile from current source. This
# removes only separately native-tested acquisition wiring from old whole-file
# comparisons; it never replaces or skips a functional regression.
function Get-SheilunScopeBlock([string]$source,[string]$start){
 $at=$source.IndexOf($start);if($at -lt 0){throw "Missing acquisition scope boundary: $start"}
 $end=$source.IndexOf('{',$at)+1;$depth=1
 while($depth -and $end -lt $source.Length){$depth+=([int]($source[$end] -eq '{'))-([int]($source[$end] -eq '}'));++$end}
 if($depth){throw "Unclosed acquisition scope boundary: $start"};$source.Substring($at,$end-$at)
}
function Assert-SheilunScopeEqual([string]$actual,[string]$expected,[string]$label){
 if(($actual -replace '\s','') -cne ($expected -replace '\s','')){throw "Unexpected acquisition delta: $label"}
}
function Get-SheilunAcquisitionRetainedMap([string]$Source){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $path='src/server/game/Maps/Map.cpp'
 $pinned=(& git -C $repo show "c3c6e4f8c:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-acquisition Map unavailable'}
 $delta=@'
    // Sheilun's two native202828 kites install Tak-Tak during AddToWorld,
    // before IsSummonedBy. Bind here so the ordinary accessory summon
    // inherits the same accepted quest/privacy scope before its first create.
    if (entry == 102648 && (GetId() == 1541 || GetId() == 1514) && summoner && summoner->IsPlayer())
    {
        Player* owner = summoner->ToPlayer();
        QuestStatus status = owner->GetQuestStatus(41003);
        if (owner->getClass() == CLASS_MONK && !owner->isGameMaster() && !owner->IsQuestRewarded(41003) &&
            (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE))
            owner->BindQuestObject(41003, summon);
    }
'@
 $normalized=$Source.Replace("`r",'')
 if([regex]::Matches($normalized,[regex]::Escape($delta)).Count -ne 1){throw 'Exact Sheilun pre-create scope binding missing/duplicated'}
 $restored=$normalized.Replace($delta,'')
 Assert-SheilunScopeEqual $restored $pinned 'entire Map differs only by the independently native-tested early102648 scope binding'
 return $restored
}
function Get-SheilunAcquisitionRetainedSource([string]$Source){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp'
 $baseline=(& git -C $repo show "ab99b2d2d:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-acquisition source unavailable'}
 $current=if($Source){$Source.Replace("`r",'')}else{(Get-Content "$repo/$path" -Raw).Replace("`r",'')}
 $restored=$current
 foreach($include in @('sheilun_acquisition.h','sheilun_return.h','sheilun_acquisition_scripts.h')){
  $line='#include "'+$include+'"'
  if([regex]::Matches($current,[regex]::Escape($line)).Count -ne 1){throw 'Acquisition include registration missing/duplicated'}
  $restored=$restored.Replace($line,'')
 }
 $boss=Get-SheilunScopeBlock $current 'class npc_aspersiy :'
 $oldBoss=Get-SheilunScopeBlock $baseline 'class npc_aspersiy :'
 $enter=@'
void EnterCombat(Unit* who) override
{
    events.RescheduleEvent(EVENT_1, 21000); // 204779
    events.RescheduleEvent(EVENT_2, 35000); // 204773  21
    events.RescheduleEvent(EVENT_3, 36000); // 43 summons
    for (uint32 entry : {101881, 101882, 101885, 101883})
        if (Creature* ally = me->FindNearestCreature(entry, 100.0f, true))
            if (CreatureAI* ai = ally->AI())
            {
                SheilunAcquisition::BattleAlly(me, ally);
                ai->AttackStart(me);
            }
}
'@
 $death=@'
void JustDied(Unit* who) override
{
    SheilunAcquisition::Defeated(me, who);
}
'@
 $expected=$oldBoss.Replace((Get-SheilunScopeBlock $oldBoss 'void EnterCombat('),$enter).Replace((Get-SheilunScopeBlock $oldBoss 'void JustDied('),$death)
 Assert-SheilunScopeEqual $boss $expected 'boss retains all original spells/events/damage/summons; only recorded ally dispatch and deduplicated instance death observer differ'
 $restored=$restored.Replace($boss,$oldBoss)
 $go=Get-SheilunScopeBlock $current 'class go_art_sheylun :'
 $expectedGo=@'
class go_art_sheylun : public GameObjectScript
{
public:
    go_art_sheylun() : GameObjectScript("go_art_sheylun") {}
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (SheilunAcquisition::Instance* controller = SheilunAcquisition::Controller(player)) controller->Open(player, go);
        return true;
    }
}
'@
 Assert-SheilunScopeEqual $go $expectedGo 'GO activation grants replaced solely by owned native loot opener'
 $restored=$restored.Replace($go,(Get-SheilunScopeBlock $baseline 'class go_art_sheylun :'))
 $registration=Get-SheilunScopeBlock $current 'void AddSC_Sheylun('
 $oldRegistration=Get-SheilunScopeBlock $baseline 'void AddSC_Sheylun('
 Assert-SheilunScopeEqual $registration ($oldRegistration.Replace('    new go_art_sheylun();',"    new go_art_sheylun();`n    SheilunAcquisition::Register();`n    RegisterInstanceScript(instance_sheylun, 1541);")) 'single controller and moved exact-name instance factory registration'
 $restored=$restored.Replace($registration,$oldRegistration)
 Assert-SheilunScopeEqual $restored $baseline 'entire remaining Sheylun.cpp'
 $shared='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp'
 $oldShared=(& git -C $repo show "ab99b2d2d:$shared") -join "`n"
 if($LASTEXITCODE){throw 'Pinned shared factory source unavailable'}
 $currentShared=Get-Content "$repo/$shared" -Raw
 $oldFactory=Get-SheilunScopeBlock $oldShared 'struct instance_sheylun :'
 $expectedShared=$oldShared.Replace('// Sheilun artifact acquisition - map 1541, scenario 1007','').Replace($oldFactory+';','').Replace('    RegisterInstanceScript(instance_sheylun, 1541);','')
 Assert-SheilunScopeEqual $currentShared $expectedShared 'shared scenario factories changed only by moving the former empty Sheilun factory'
 $scripts=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_acquisition_scripts.h" -Raw
 $factory=Get-SheilunScopeBlock $scripts 'struct instance_sheylun :'
 Assert-SheilunScopeEqual $factory 'struct instance_sheylun : SheilunAcquisition::Instance { explicit instance_sheylun(InstanceMap* map) : Instance(map) { } }' 'DB factory spelling and controller inheritance'
 return $restored
}
function Get-SheilunAcquisitionRetainedPlan([string]$Source){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 [void](Get-SheilunAcquisitionRetainedSource)
 $path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_plan.h'
 $prior=(& git -C $repo show "ab99b2d2d:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-acquisition plan unavailable'}
 $expected=$prior.Replace('#define SHEILUN_PLAN_H',"#define SHEILUN_PLAN_H`n`n#include `"sheilun_acquisition_api.h`"")
 $expected=$expected.Replace('        !Planned(scenario) || !Receipt(scenario, 46662)) return;',"        !Planned(scenario) || !Receipt(scenario, 46662)) return;`n    SheilunAcquisition::Started(player);")
 Assert-SheilunScopeEqual $Source $expected 'plan adds only the controller observer after native46662/current-attempt acknowledgement'
 return $Source.Replace('#include "sheilun_acquisition_api.h"','').Replace('    SheilunAcquisition::Started(player);','')
}
