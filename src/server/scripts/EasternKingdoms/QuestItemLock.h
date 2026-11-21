#ifndef EASTERN_KINGDOMS_QUEST_ITEM_LOCK_H
#define EASTERN_KINGDOMS_QUEST_ITEM_LOCK_H

#include "GameObject.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "Player.h"
#include "Spell.h"
#include "SpellScript.h"

class QuestItemLockAI : public GameObjectAI
{
public:
    QuestItemLockAI(GameObject* object, uint32 opening, uint32 item) : GameObjectAI(object), _opening(opening), _item(item) { }

    bool CanOpenWith(Player const* player, uint32 spell, Item const* item) const
    {
        return spell == _opening && item && item->GetEntry() == _item && CanOpen(player);
    }

    void BeginOpening(Player const* player, uint32 spell, Item const* item)
    {
        _opener = CanOpenWith(player, spell, item) ? player->GetGUID() : ObjectGuid::Empty;
    }

    void EndOpening() { _opener.Clear(); }

    bool GossipUse(Player* player) final
    {
        if (_opener != player->GetGUID())
            return true;
        EndOpening();
        return !CanOpen(player) || !OnOpened(player);
    }

protected:
    virtual bool CanOpen(Player const* player) const = 0;
    virtual bool OnOpened(Player* /*player*/) { return true; }

private:
    uint32 const _opening;
    uint32 const _item;
    ObjectGuid _opener;
};

class spell_quest_item_opening : public SpellScript
{
    PrepareSpellScript(spell_quest_item_opening);

    static QuestItemLockAI* LockAI(GameObject* object)
    {
        return object ? dynamic_cast<QuestItemLockAI*>(object->AI()) : nullptr;
    }

    SpellCastResult CheckOpening()
    {
        auto lock = LockAI(GetExplTargetGObj());
        return lock && lock->CanOpenWith(GetCaster()->ToPlayer(), GetSpellInfo()->Id, GetCastItem())
            ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    // The default OPEN_LOCK handler still checks Lock.db2 before calling
    // GameObject::Use synchronously. Only that use is authorized.
    void BeginOpening(SpellEffIndex /*effect*/)
    {
        if (auto lock = LockAI(GetHitGObj()))
            lock->BeginOpening(GetCaster()->ToPlayer(), GetSpellInfo()->Id, GetCastItem());
    }

    void EndOpening()
    {
        if (auto lock = LockAI(GetHitGObj()))
            lock->EndOpening();
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_quest_item_opening::CheckOpening);
        OnEffectHitTarget += SpellEffectFn(spell_quest_item_opening::BeginOpening, EFFECT_0, SPELL_EFFECT_OPEN_LOCK);
        AfterHit += SpellHitFn(spell_quest_item_opening::EndOpening);
    }
};

#endif
