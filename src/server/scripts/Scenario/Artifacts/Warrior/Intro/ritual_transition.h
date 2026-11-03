#pragma once

namespace WarriorRitual
{
    constexpr uint32 AIStatusData = 93330, DanicaData = 93331, HitData = 93332, SecondBattleData = 93334;

    // The instance retains real deaths/normal summon receipts when an actor's
    // combat AI is replaced. Neither a summon nor an arbitrary spell hit is
    // evidence that both original warriors have ascended.
    struct State
    {
        explicit State(Map* owner) : map(owner) { }
        Map* map;
        ObjectGuid source, danica, ragnvald, ally, nextEnemy, lastDeath;
        ObjectGuid corpses[2], spirits[2], candidates[2];
        std::vector<MalgalorFinale::Member> members;
        uint64 elapsed = 0, preparedAt = 0, departAt = 0;
        uint32 retry = 0;
        bool pending = false, prepared = false, spoken = false, castAccepted = false;
        bool castingCorpse[2] = {}, castReturned[2] = {};
        bool reachedStageTwo = false, departed = false, resumed = false;
        bool actorWarned = false, castWarned = false, spiritWarned = false, creditWarned = false, resumeWarned = false;

        Scenario* Progress() const
        {
            if (!map || map->IsMapUnload() || map->GetId() != 1500) return nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 933 ? scenario : nullptr;
        }

        bool Leaf(uint32 id) const
        {
            Scenario* scenario = Progress();
            if (!scenario) return false;
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(id);
            return found != progress->end() && found->second.Counter;
        }

        Creature* Native(ObjectGuid guid, uint32 entry, uint32 spawn, bool alive = true) const
        {
            Creature* actor = map ? map->GetCreature(guid) : nullptr;
            return actor && actor->GetMap() == map && actor->IsInWorld() && !actor->IsPreDelete() &&
                actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn && actor->IsAlive() == alive ? actor : nullptr;
        }

        Creature* Owned(ObjectGuid guid, uint32 entry, ObjectGuid owner, uint32 spell) const
        {
            Creature* actor = map ? map->GetCreature(guid) : nullptr;
            TempSummon* summon = actor ? actor->ToTempSummon() : nullptr;
            return actor && summon && actor->GetMap() == map && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() &&
                actor->GetEntry() == entry && summon->GetSummonerGUID() == owner &&
                actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == spell ? actor : nullptr;
        }

        Creature* Danica() const { return Owned(danica, 103608, source, 0); }

        bool Retained(MalgalorFinale::Member const& member) const
        {
            Player* player = ObjectAccessor::FindPlayer(member.player);
            auto status = player ? player->getQuestStatus(member.quest) : nullptr;
            auto attempt = member.attempt.lock();
            return player && status && attempt && status->ScriptLifetime == attempt && player->GetMap() == map &&
                player->getClass() == CLASS_WARRIOR && MalgalorFinale::Quest(player) == member.quest &&
                player->GetQuestStatus(member.quest) == QUEST_STATUS_INCOMPLETE && !player->GetQuestObjectiveData(member.quest, 103661);
        }

        Player* Current(MalgalorFinale::Member const& member) const
        {
            Player* player = MalgalorFinale::Original(member);
            Creature* actor = Danica();
            return Retained(member) && player && !player->HasPendingTeleport() && actor && player->InSamePhase(actor) ? player : nullptr;
        }

        bool Ready() const
        {
            Scenario* scenario = Progress();
            return pending && scenario && scenario->GetCurrentStep() == 2 && Leaf(45771) && Leaf(46175) &&
                std::any_of(members.begin(), members.end(), [this](MalgalorFinale::Member const& member) { return Current(member) != nullptr; });
        }

        void Observe(Creature* actor)
        {
            if (!actor || actor->GetMap() != map) return;
            if (actor->GetEntry() == 102970 && actor->GetDBTableGUIDLow() == 361138 && actor->IsAlive() && !pending)
                lastDeath.Clear();
            if (actor->GetEntry() == 97167 && actor->GetDBTableGUIDLow() == 361094) ragnvald = actor->GetGUID();
            if (actor->GetEntry() == 97166 && actor->GetDBTableGUIDLow() == 361091) ally = actor->GetGUID();
            if (actor->GetEntry() == 98257 && actor->GetDBTableGUIDLow() == 361151) nextEnemy = actor->GetGUID();
            for (uint8 i = 0; i < 2; ++i)
            {
                if ((!prepared || corpses[i].IsEmpty()) && actor->GetEntry() == 102964u + i && actor->GetDBTableGUIDLow() == 361098u + i)
                    corpses[i] = actor->GetGUID();
                // Map::AddToMap invokes this before CastSpell returns. Do not
                // accept that tentative child until the normal cast succeeds.
                if (castingCorpse[i] && candidates[i].IsEmpty() &&
                    Owned(actor->GetGUID(), 103688u - i, corpses[i], 205295u + i))
                    candidates[i] = actor->GetGUID();
            }
            if (actor->GetEntry() != 103608 || (!danica.IsEmpty() && (pending || Danica()))) return;
            TempSummon* summon = actor->ToTempSummon();
            Creature* owner = summon ? Native(summon->GetSummonerGUID(), 102970, 361138, false) : nullptr;
            Scenario* scenario = Progress();
            if (owner && (pending || lastDeath != owner->GetGUID()) && scenario && scenario->GetCurrentStep() <= 2 && (source.IsEmpty() || source == owner->GetGUID()) &&
                actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 0)
            { source = owner->GetGUID(); danica = actor->GetGUID(); }
        }

        void Death(Creature* actor, Unit* killer)
        {
            Scenario* scenario = Progress();
            if (pending || !scenario || scenario->GetCurrentStep() > 1 || !actor || lastDeath == actor->GetGUID() ||
                actor->GetMap() != map || Native(actor->GetGUID(), 102970, 361138, false) != actor ||
                actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1)) return;
            members.clear();
            for (auto const& reference : map->GetPlayers())
                if (Player* player = reference.getSource())
                {
                    uint32 quest = MalgalorFinale::Quest(player);
                    if (!quest || player->GetQuestStatus(quest) != QUEST_STATUS_INCOMPLETE || player->getClass() != CLASS_WARRIOR ||
                        player->isGameMaster() || !player->IsInWorld() || !player->CanContact() || player->HasPendingTeleport() ||
                        player->GetMap() != map || !player->InSamePhase(actor) || player->GetQuestObjectiveData(quest, 103661)) continue;
                    player->AddQuestDelayedEvent(quest, 0, [] { });
                    auto status = player->getQuestStatus(quest);
                    if (status && status->ScriptLifetime)
                        members.push_back({player->GetGUID(), quest, actor->GetInstanceId(), 0, status->ScriptLifetime});
                }
            if (members.empty()) return;
            Player* owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (owner)
                for (auto member = members.begin(); member != members.end(); ++member)
                    if (member->player == owner->GetGUID()) { std::iter_swap(members.begin(), member); break; }
            source = lastDeath = actor->GetGUID();
            if (!Owned(danica, 103608, source, 0)) danica.Clear();
            elapsed = preparedAt = departAt = retry = 0;
            prepared = spoken = castAccepted = reachedStageTwo = departed = resumed = false;
            actorWarned = castWarned = spiritWarned = creditWarned = resumeWarned = false;
            for (uint8 i = 0; i < 2; ++i)
            { spirits[i].Clear(); candidates[i].Clear(); castingCorpse[i] = castReturned[i] = false; }
            pending = true;
        }

        void Cancel()
        {
            pending = false;
            members.clear();
            if (Creature* actor = Danica()) actor->DespawnOrUnsummon();
            for (uint8 i = 0; i < 2; ++i)
                for (ObjectGuid guid : {spirits[i], candidates[i]})
                    if (Creature* actor = Owned(guid, 103688u - i, corpses[i], 205295u + i)) actor->DespawnOrUnsummon();
            danica.Clear(); source.Clear();
            prepared = spoken = castAccepted = reachedStageTwo = departed = resumed = false;
        }

        void AcceptSpirit(uint8 i)
        {
            if (spirits[i].IsEmpty() && castReturned[i] && Owned(candidates[i], 103688u - i, corpses[i], 205295u + i))
            { spirits[i] = candidates[i]; castingCorpse[i] = false; retry = 0; }
        }

        void Hit(ObjectGuid target)
        {
            if (!castAccepted || !Ready()) return;
            for (uint8 i = 0; i < 2; ++i)
                if (target == corpses[i] && spirits[i].IsEmpty() && !castingCorpse[i])
                    if (Creature* corpse = Native(target, 102964u + i, 361098u + i, false))
                    {
                        castingCorpse[i] = true;
                        candidates[i].Clear(); castReturned[i] = false;
                        auto result = corpse->CastSpell(corpse, 205295u + i, false);
                        // The instance survives arbitrary AI replacement during
                        // the external native cast. Re-resolve all live objects.
                        castReturned[i] = result == SPELL_CAST_OK;
                        if (castReturned[i] && pending) AcceptSpirit(i);
                        else castingCorpse[i] = false;
                        if (!castReturned[i] || !pending)
                        {
                            if (Creature* actor = Owned(candidates[i], 103688u - i, corpses[i], 205295u + i)) actor->DespawnOrUnsummon();
                            candidates[i].Clear();
                        }
                        // Matching26972 summons have zero cast/GCD/missile
                        // delay and execute during prepare. A refused/missing
                        // child is not an accepted receipt or an infinite wait.
                        if (spirits[i].IsEmpty()) castingCorpse[i] = false;
                    }
        }

        bool Speak()
        {
            Creature* actor = Danica();
            if (!actor || !Ready()) return false;
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1405, actor, nullptr, *actor))
            { delete conversation; return false; }
            actor = Danica();
            if (!actor || !Ready() || !conversation->IsInWorld() || ObjectAccessor::GetConversation(*actor, conversation->GetGUID()) != conversation ||
                conversation->GetCreatures().size() != 2)
            { conversation->Remove(); return false; }
            auto participant = conversation->GetCreature(0);
            auto second = conversation->GetCreature(1);
            Creature* original = Native(ragnvald, 97167, 361094);
            if (!participant || !second || !original || !original->InSamePhase(actor) || participant->GuidLow != danica.GetLowPart() || participant->GuidHight != danica.GetHighPart() ||
                second->GuidLow != ragnvald.GetLowPart() || second->GuidHight != ragnvald.GetHighPart())
            { conversation->Remove(); return false; }
            return true;
        }

        void Prepare()
        {
            Creature* actor = Danica();
            if (!actor || !Ready()) return;
            prepared = true; preparedAt = elapsed;
            std::list<Creature*> trash;
            for (uint32 entry : {102964u, 102965u, 92445u, 92446u, 92450u, 92447u, 97176u})
                GetCreatureListWithEntryInGrid(trash, actor, entry, 90.0f);
            std::vector<ObjectGuid> targets;
            for (Creature* target : trash) targets.push_back(target->GetGUID());
            for (ObjectGuid guid : targets)
            {
                if (!Ready()) return;
                if (Creature* target = map->GetCreature(guid))
                    if (target->IsInWorld() && !target->IsPreDelete() && target->IsAlive()) target->Kill(target);
            }
            if (!Ready()) return;
            if (Creature* target = Native(ally, 97166, 361091))
                if (target->InSamePhase(actor) && actor->IsWithinDistInMap(target, 100.0f)) sCreatureTextMgr->SendChat(target, 0);
            actor = Danica();
            if (!actor) return;
            actor->GetMotionMaster()->MovePoint(0, -1980.04f, -1251.96f, 4.83f);
            std::list<Creature*> adds;
            GetCreatureListWithEntryInGrid(adds, actor, 102990, 160.0f);
            for (uint32 entry : {102995u, 97166u, 97167u}) GetCreatureListWithEntryInGrid(adds, actor, entry, 90.0f);
            for (Creature* target : adds)
                if (target->IsAlive() && !target->HasAura(105419))
                    target->GetMotionMaster()->MovePoint(0, -1965.64f + irand(-6, 6), -1246.74f + irand(-6, 6), 2.71f);
        }

        void Depart()
        {
            departed = true;
            for (uint8 i = 0; i < 2; ++i)
                if (Creature* actor = Owned(spirits[i], 103688u - i, corpses[i], 205295u + i))
                {
                    actor->GetMotionMaster()->MovePoint(0, actor->GetPositionX(), actor->GetPositionY(), 40.0f);
                    actor->DespawnOrUnsummon(4000);
                }
            if (Creature* actor = Danica())
            {
                actor->GetMotionMaster()->MovePoint(0, actor->GetPositionX(), actor->GetPositionY(), 40.0f);
                actor->DespawnOrUnsummon(4000);
            }
        }

        void Update(uint32 diff)
        {
            if (!pending)
            {
                if (Native(lastDeath, 102970, 361138)) lastDeath.Clear();
                return;
            }
            Scenario* scenario = Progress();
            if (!scenario || scenario->GetCurrentStep() > 3 ||
                std::none_of(members.begin(), members.end(), [this](MalgalorFinale::Member const& member) { return Retained(member); }))
            { Cancel(); return; }
            elapsed += diff;
            // No entrance cleanup, corpse death, cast or later-tree event
            // while the genuine earlier death waits for native prerequisites.
            if (scenario->GetCurrentStep() < 2)
            { if (reachedStageTwo) Cancel(); return; }
            reachedStageTwo = true;
            if (scenario->GetCurrentStep() == 3)
            {
                if (spirits[0].IsEmpty() || spirits[1].IsEmpty() || !Leaf(46912) || !Leaf(46929)) return;
                if (!departAt) { departAt = elapsed + 2000; return; }
                if (elapsed < departAt) return;
                if (!departed) Depart();
                Creature* actor = Native(ragnvald, 97167, 361094);
                Creature* enemy = Native(nextEnemy, 98257, 361151);
                CreatureAI* ai = actor && actor->IsAIEnabled ? actor->AI() : nullptr;
                uint32 status = ai ? ai->GetData(WarriorOpening::AIStatusData) : 0;
                if (enemy && actor && actor->InSamePhase(enemy) && actor->IsWithinDistInMap(enemy, 250.0f) &&
                    (status == WarriorOpening::Idle || status == WarriorOpening::Queued))
                {
                    resumed = true; pending = false;
                    if (auto script = actor->GetInstanceScript()) script->SetGuidData(SecondBattleData, actor->GetGUID());
                    members.clear(); ai->DoAction(true); return;
                }
                if (!resumeWarned) TC_LOG_WARN("scripts", "Warrior ritual: earned stage3 has no original Ragnvald AI/Bezzeredes target; retaining handoff retry");
                resumeWarned = true;
                return;
            }
            if (prepared && elapsed - preparedAt < 3000) return;
            if (retry > diff) { retry -= diff; return; }
            retry = 1000;
            if (!Danica())
            {
                // Match only the failed native Smart action12, preserving its
                // real dead summoner, manual type, coordinates and properties.
                if (danica.IsEmpty())
                    if (Creature* corpse = Native(source, 102970, 361138, false))
                        corpse->SummonCreature(103608, -2001.0f, -1255.0f, 37.0f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0);
            }
            if (!Ready())
            {
                if (!actorWarned) TC_LOG_WARN("scripts", "Warrior ritual: genuine death has no original live Danica/eligible participant; retaining native retry");
                actorWarned = true; return;
            }
            actorWarned = false;
            if (!prepared) { Prepare(); retry = 0; return; }
            if (!spoken) spoken = Speak();
            Creature* actor = Danica();
            if (!spoken || !actor || !Ready())
            {
                if (!castWarned) TC_LOG_WARN("scripts", "Warrior ritual: native1405 not acknowledged; no cast/credit, retaining retry");
                castWarned = true; return;
            }
            for (uint8 i = 0; i < 2; ++i) AcceptSpirit(i);
            if (spirits[0].IsEmpty() || spirits[1].IsEmpty())
            {
                if (actor->HasUnitState(UNIT_STATE_CASTING)) return;
                // A real early/ranged demon death can leave the two warriors
                // outside the original90yd cleanup at Danica's high spawn.
                // Retry only those original actors after her native movement
                // brings them into that same range; never kill a replacement.
                for (uint8 i = 0; i < 2; ++i)
                    if (spirits[i].IsEmpty())
                        if (Creature* corpse = Native(corpses[i], 102964u + i, 361098u + i))
                            if (corpse->InSamePhase(actor) && actor->IsWithinDistInMap(corpse, 90.0f)) corpse->Kill(corpse);
                actor = Danica();
                if (!actor || !Ready()) return;
                if (castAccepted && !spiritWarned)
                    TC_LOG_WARN("scripts", "Warrior ritual: native205287 has no two original corpse/spirit receipts; retaining only missing summons");
                spiritWarned = castAccepted;
                CreatureAI* ai = actor->IsAIEnabled ? actor->AI() : nullptr;
                if (!ai || ai->GetData(AIStatusData) != 1) return;
                castAccepted = actor->CastSpell(actor, 205287, false) == SPELL_CAST_OK;
                if (!castAccepted && !castWarned) TC_LOG_WARN("scripts", "Warrior ritual: normal205287 refused; retaining original corpse retry");
                castWarned = !castAccepted;
                return;
            }
            retry = 2000;
            auto participants = members;
            for (auto const& member : participants)
                if (Player* player = Current(member))
                {
                    if (!Leaf(46912)) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49203);
                    if (!pending) return;
                    scenario = Progress(); player = Current(member);
                    if (scenario && scenario->GetCurrentStep() == 2 && player && Leaf(46912) && !Leaf(46929))
                        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49262);
                    if (Leaf(46912) && Leaf(46929)) { departAt = elapsed + 2000; return; }
                }
            if (!creditWarned) TC_LOG_WARN("scripts", "Warrior ritual: both real spirits exist but native49203/49262 not acknowledged; retaining missing-only retry");
            creditWarned = true;
        }
    };
}
