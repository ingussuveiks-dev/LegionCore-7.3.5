#include "ScriptMgr.h"
#include "SmartAI.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "Pet.h"
#include "Item.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"

namespace RitualRuination
{
constexpr uint32 QuestId=43254, Artifact=128941, Flight=219917;
uint32 const Objectives[]={110523,110581,110556,110603,110606,110607};
Position const Landing={-666.047f,2613.08f,81.0715f,5.32719f};
Position const BatSource={-818.038f,4258.45f,746.367f,3.16069f};
Position const Scepter={-656.67f,2600.33f,82.5702f,0};
Position const Image={-622.809f,2578.51f,88.3115f,2.49362f};
// Existing phase7027 prop position: supported ground beside the ritual route.
Position const Escape={-731.993f,2564.75f,77.4463f,0};
// Existing native220374 destination, shared with the previous artifact quest.
Position const Dalaran={-835.26f,4277.1f,746.25f,5.52292f};

bool Active(Player* p)
{
    return p && p->getClass()==CLASS_WARLOCK && !p->GetQuestRewardStatus(QuestId) &&
        (p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE);
}
uint32 Stage(Player* p)
{
    if (!Active(p)) return 7;
    for (uint32 i=0;i<6;++i) if (!p->GetQuestObjectiveData(QuestId,Objectives[i])) return i;
    return 6;
}
bool Ready(Player* p)
{
    return Active(p) && p->IsInWorld() && p->IsAlive() && !p->IsBeingTeleported() &&
        !p->GetVehicle() && !p->GetTransport() && !p->isInFlight();
}
bool AtRitual(Player* p)
{
    return Ready(p) && p->GetMapId()==1220 && p->GetZoneId()==7543 && p->GetExactDist(&Landing)<180.0f;
}
void FinishLanding(Player* p)
{
    if (!AtRitual(p) || !p->HasAura(Flight)) return;
    // The old SAI teleported immediately but left the five-minute native
    // summon-controller aura active. End only this quest's interrupted ride.
    p->RemoveAurasDueToSpell(Flight);
    p->StopMoving();
    p->RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING_OR_TURN);
    p->SetClientControl(p,true); // Native control checks retain independent roots.
    p->ResummonPetTemporaryUnSummonedIfAny();
    p->GetPhaseMgr().NotifyStoresReloaded();
    TC_LOG_INFO("entities.player", "Ritual Ruination: ended stale Fel Bat controller for player %u; restored self mover",p->GetGUIDLow());
}
void ConfirmLanding(Player* p,uint32 retries)
{
    if (!Active(p)) return;
    if (AtRitual(p) && p->GetExactDist(&Landing)<12.0f)
    {
        if (Stage(p)==0) p->KilledMonsterCredit(Objectives[0],ObjectGuid::Empty);
        FinishLanding(p);
        return;
    }
    if (retries) p->AddQuestDelayedEvent(QuestId,500,[p,retries](){ConfirmLanding(p,retries-1);});
}
bool StartTravel(Player* p)
{
    if (!Ready(p) || p->GetMapId()!=1220 || p->GetExactDist(&BatSource)>12.0f || Stage(p)>5) return false;
    // Preserve the existing direct transfer. A same-map zone crossing must
    // use the far/zone lifecycle so pets and the destination grid move too.
    p->StopMoving();
    p->RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING_OR_TURN);
    if (!p->TeleportTo(1220,&Landing,TELE_TO_ZONE_MAP)) return false;
    p->AddQuestDelayedEvent(QuestId,500,[p](){ConfirmLanding(p,60);});
    return true;
}
bool CanPickup(Player* p)
{
    return AtRitual(p) && Stage(p)==3 && p->GetExactDist(&Scepter)<8.0f;
}
bool CanRuin(Player* p)
{
    return AtRitual(p) && Stage(p)==4 && p->HasItemCount(Artifact,1) && p->GetExactDist(&Scepter)<8.0f;
}
Creature* Owned(Player* p,uint32 entry)
{
    // Direct WorldObject summons have no SummonProperties. InitStats returns
    // before inserting them into Unit::tempSummonList, so that list cannot
    // prove absence. Inspect real, quest-bound actors and their summoner.
    std::list<Creature*> actors;
    p->GetCreatureListWithEntryInGrid(actors,entry,300.0f);
    Creature* found=nullptr;
    for (Creature* c:actors)
        if (TempSummon* summon=c->ToTempSummon())
            if (c->IsAlive() && !c->IsQuestObjectExpired() && c->HasQuestObjectScope() &&
                summon->GetSummonerGUID()==p->GetGUID())
            {
                if (!found) found=c;
                else c->DespawnOrUnsummon(); // Repair duplicates from an interrupted attempt, scoped to this owner.
            }
    return found;
}
Creature* Personal(Player* p,uint32 entry,Position const& pos)
{
    if (Creature* c=Owned(p,entry)) return c;
    if (Creature* c=p->SummonCreature(entry,pos,TEMPSUMMON_MANUAL_DESPAWN,0,0,p->GetGUID()))
    {
        p->BindQuestObject(QuestId,c);
        TC_LOG_INFO("entities.player","Ritual Ruination: created quest-bound actor %u for player %u",entry,p->GetGUIDLow());
        return c;
    }
    return nullptr;
}
void Recover(Player* p)
{
    if (!Active(p)) return;
    FinishLanding(p);
    if (AtRitual(p))
    {
        if (Stage(p)==3)
            if (Creature* image=Personal(p,110548,Image)) image->SetPhaseId(p->GetPhases(),true);
        if (Stage(p)==5)
            if (Creature* portal=Personal(p,110800,Escape)) portal->SetPhaseId(p->GetPhases(),true);
    }
    if (Ready(p) && Stage(p)==6 && p->GetMapId()==1220 && p->GetExactDist(&Dalaran)<30.0f)
        Personal(p,101097,Dalaran);
}
void ConfirmEscape(Player* p,uint32 retries)
{
    if (!Active(p)) return;
    if (Ready(p) && p->GetMapId()==1220 && p->GetExactDist(&Dalaran)<12.0f)
    {
        if (Stage(p)==5) p->KilledMonsterCredit(Objectives[5],ObjectGuid::Empty);
        Recover(p);
        return;
    }
    if (retries) p->AddQuestDelayedEvent(QuestId,500,[p,retries](){ConfirmEscape(p,retries-1);});
}
}

class spell_ritual_fel_bat : public SpellScript
{
    PrepareSpellScript(spell_ritual_fel_bat);
    SpellCastResult Check()
    {
        Player* p=GetCaster()->ToPlayer();
        Unit* bat=GetExplTargetUnit();
        return RitualRuination::Ready(p) && p->GetMapId()==1220 && p->GetExactDist(&RitualRuination::BatSource)<12.0f &&
            bat && bat->GetEntry()==110479 && p->InSamePhase(bat) && p->GetExactDist(bat)<8.0f &&
            RitualRuination::Stage(p)<=5 ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void PreventRide(SpellEffIndex i){PreventHitDefaultEffect(i);}
    void Travel(){RitualRuination::StartTravel(GetCaster()->ToPlayer());}
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_ritual_fel_bat::Check);
        OnEffectHitTarget+=SpellEffectFn(spell_ritual_fel_bat::PreventRide,EFFECT_0,SPELL_EFFECT_APPLY_AURA);
        OnEffectHitTarget+=SpellEffectFn(spell_ritual_fel_bat::PreventRide,EFFECT_1,SPELL_EFFECT_TRIGGER_SPELL);
        AfterCast+=SpellCastFn(spell_ritual_fel_bat::Travel);
    }
};
class go_ritual_scepter : public GameObjectScript
{
public:
    go_ritual_scepter():GameObjectScript("go_ritual_scepter"){}
    bool OnGossipHello(Player* p,GameObject* go) override
    {
        if (!RitualRuination::CanPickup(p) || !p->InSamePhase(go) || !p->canSeeOrDetect(go) ||
            !p->IsWithinLOSInMap(go) || p->GetExactDist(go)>5.0f) return true;
        if (p->HasItemCount(RitualRuination::Artifact,1))
        {
            p->CastSpell(p,219839,true);
            return true;
        }
        return false; // Native chest loot must actually enter the inventory first.
    }
};
class spell_ritual_artifact : public SpellScript
{
    PrepareSpellScript(spell_ritual_artifact);
    SpellCastResult Check(){Player* p=GetCaster()->ToPlayer();return RitualRuination::CanPickup(p) && p->HasItemCount(RitualRuination::Artifact,1) ? SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
    void Register() override {OnCheckCast+=SpellCheckCastFn(spell_ritual_artifact::Check);}
};
class spell_ritual_altar : public SpellScript
{
    PrepareSpellScript(spell_ritual_altar);
    SpellCastResult Check(){return RitualRuination::CanRuin(GetCaster()->ToPlayer())?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
    void Register() override {OnCheckCast+=SpellCheckCastFn(spell_ritual_altar::Check);}
};
class spell_ritual_escape : public SpellScript
{
    PrepareSpellScript(spell_ritual_escape);
    bool scoped=false;
    SpellCastResult Check()
    {
        Player* p=GetCaster()->ToPlayer();
        scoped=p && p->GetMapId()==1220 && p->GetZoneId()==7543;
        if (!scoped) return SPELL_CAST_OK; // Also used by scenario1155.
        Creature* portal=GetExplTargetUnit()?GetExplTargetUnit()->ToCreature():nullptr;
        TempSummon* summon=portal?portal->ToTempSummon():nullptr;
        return RitualRuination::AtRitual(p) && RitualRuination::Stage(p)==5 && summon &&
            portal->GetEntry()==110800 && summon->GetSummonerGUID()==p->GetGUID() &&
            p->InSamePhase(portal) && p->GetExactDist(portal)<8.0f ? SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Credit(SpellEffIndex i){if(scoped)PreventHitDefaultEffect(i);}
    void After(){if(scoped){Player* p=GetCaster()->ToPlayer();p->AddQuestDelayedEvent(RitualRuination::QuestId,500,[p](){RitualRuination::ConfirmEscape(p,60);});}}
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_ritual_escape::Check);
        OnEffectHitTarget+=SpellEffectFn(spell_ritual_escape::Credit,EFFECT_1,SPELL_EFFECT_KILL_CREDIT);
        AfterCast+=SpellCastFn(spell_ritual_escape::After);
    }
};
struct npc_ritual_guldan_image : ScriptedAI
{
    explicit npc_ritual_guldan_image(Creature* c):ScriptedAI(c){}
    ObjectGuid owner;uint32 timer=10000;bool final=false,said=false;
    void IsSummonedBy(Unit* summoner) override
    {
        Player* p=summoner?summoner->ToPlayer():nullptr;
        if (!RitualRuination::AtRitual(p)) return;
        owner=p->GetGUID();final=RitualRuination::Stage(p)>=5;
        Talk(final?2:0,owner);
        if(final)p->CastSpell(p,219927,true); // Native conversation2966, independent of portal recovery.
    }
    void UpdateAI(uint32 diff) override
    {
        Player* p=ObjectAccessor::GetPlayer(*me,owner);
        if(!RitualRuination::AtRitual(p) || (!final && RitualRuination::Stage(p)!=3)){me->DespawnOrUnsummon();return;}
        if(!final && !said){if(diff>=timer){said=true;Talk(1,owner);}else timer-=diff;}
    }
};
class player_ritual_ruination : public PlayerScript
{
public:
    player_ritual_ruination():PlayerScript("player_ritual_ruination"){}
    void OnUpdate(Player* p,uint32) override {RitualRuination::Recover(p);}
    void OnLogout(Player* p) override {p->CancelQuestObjects(RitualRuination::QuestId);}
    void OnMapChanged(Player* p) override {p->CancelQuestObjects(RitualRuination::QuestId);}
    void OnLootItem(Player* p,Item* item,uint32) override
    {
        if(item && item->GetEntry()==RitualRuination::Artifact && RitualRuination::CanPickup(p))p->CastSpell(p,219839,true);
    }
};
void AddSC_ritual_ruination()
{
    RegisterSpellScript(spell_ritual_fel_bat);
    RegisterSpellScript(spell_ritual_artifact);
    RegisterSpellScript(spell_ritual_altar);
    RegisterSpellScript(spell_ritual_escape);
    RegisterCreatureAI(npc_ritual_guldan_image);
    new go_ritual_scepter();
    new player_ritual_ruination();
}
