#include "return_to_karazhan.h"
#include "MoveSpline.h"

#define SIZE_ONE_BOX  5.9f
#define MAX_X         3941.32f
#define MAX_Y         -2221.06
#define MIN_X         3899.15f
#define MIN_Y         -2257.75f

// 115406, 115402, 115401, 115407, 115395
class npc_karazhan_chess : public CreatureScript
{
public:
    npc_karazhan_chess() : CreatureScript("npc_karazhan_chess") {}

    struct npc_karazhan_chessAI : public Scripted_NoMovementAI
    {
        npc_karazhan_chessAI(Creature* creature) : Scripted_NoMovementAI(creature) 
        {
            me->setRegeneratingHealth(false);
            me->SetReactState(REACT_PASSIVE);
            SetCombatMovement(false);
        }
        
        EventMap events;
        
        void Reset() override
        {
            _rookMoving = _rookArrived = _rookArmed = false;
            _rookSplineId = 0;
            if (me->GetEntry() == NPC_CHESS_ROOK)
                me->InterruptNonMeleeSpells(false, 229567);
            me->setRegeneratingHealth(false);
            me->SetReactState(REACT_PASSIVE);
            SetCombatMovement(false);
        }
        
        void JustDied(Unit* who) override
        {
            _rookMoving = _rookArrived = _rookArmed = false;
            if (Creature* king = me->FindNearestCreature(NPC_CHESS_KING, 200.0f, true))
                king->CastSpell(king, 229493);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (me->GetEntry() != NPC_CHESS_ROOK)
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 6.24f);
            if (type != EFFECT_MOTION_TYPE || id != 229329 || !_rookMoving)
                return;

            _rookMoving = false;
            // Effect generators also notify when interrupted. Only the spline
            // issued by this turn, completed at its destination, may attack.
            if (!me->IsAlive() || me->GetEntry() != NPC_CHESS_ROOK ||
                me->movespline->GetId() != _rookSplineId || !me->movespline->Finalized())
                return;

            auto destination = me->movespline->FinalDestination();
            if (std::abs(destination.x - _rookDestination.GetPositionX()) > 0.1f ||
                std::abs(destination.y - _rookDestination.GetPositionY()) > 0.1f ||
                std::abs(destination.z - _rookDestination.GetPositionZ()) > 0.1f)
                return;
            _rookArrived = true;
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->GetEntry() != NPC_CHESS_ROOK)
            {
                Scripted_NoMovementAI::UpdateAI(diff);
                return;
            }
            if (!_rookArrived)
                return;
            if (!me->IsAlive() || me->movespline->GetId() != _rookSplineId)
            {
                _rookArrived = false;
                return;
            }
            // Cross-cell relocation is committed by Map after Unit's spline
            // callback. Wait for that real pose before creating ground effects.
            if (me->GetExactDist(&_rookDestination) > 0.1f)
                return;
            _rookArrived = false;
            _rookArmed = true;
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 6.24f);
            me->CastSpell(me, 229567);
        }

        void RookTiles()
        {
            // One center tile and four rays, including the farthest board edge.
            me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 229570, true);
            for (uint8 axis = 0; axis != 2; ++axis)
                for (int8 sign = -1; sign <= 1; sign += 2)
                    for (uint8 length = 1; length <= 8; ++length)
                    {
                        float x = me->GetPositionX() + (axis ? 0.0f : sign * length * SIZE_ONE_BOX);
                        float y = me->GetPositionY() + (axis ? sign * length * SIZE_ONE_BOX : 0.0f);
                        if (x <= MIN_X || x >= MAX_X || y <= MIN_Y || y >= MAX_Y)
                            break;
                        me->CastSpell(x, y, me->GetPositionZ(), 229570, true);
                    }
        }
        
        void SpellHit(Unit* caster, const SpellInfo* spell) override
        {
            if (spell->Id == 229567 && caster == me && me->GetEntry() == NPC_CHESS_ROOK &&
                me->IsAlive() && _rookArmed)
            {
                _rookArmed = false;
                RookTiles();
                return;
            }
            if (spell->Id != 229468)
                return;
            if (!me->IsAlive())
                return;
            switch(me->GetEntry())
            {
                case NPC_CHESS_HORSE: 
                {
                    ///! Struct check:
                    // choose side for move by x
                    // choose length for move by x
                    // chose side for move by y
                    // calculate length for move by y
                    // if all ok, then jump + cast
                    //!
                    int8 randx = (urand(1,2) == 1 ? -1 : 1); //for rand during different time
                    int8 randy = (urand(1,2) == 1 ? -1 : 1); //for rand during different time
                    for (uint8 up_down = 0; up_down <= 1; ++up_down)
                    {
                        int8 updown = (up_down == 0 ? -1 : 1)*randx;
                        for (uint8 length_x =1; length_x <= 2; ++length_x)
                        {
                            float switchx = me->GetPositionX() + updown*length_x*SIZE_ONE_BOX;
                            if (switchx > MIN_X && switchx < MAX_X)
                            {
                                for (uint8 right_left = 0; right_left <= 1; ++right_left)
                                {
                                    int8 rightleft = (right_left == 0 ? -1 : 1)*randy;
                                    float switchy = me->GetPositionY() + rightleft*(3-length_x)*SIZE_ONE_BOX;
                                    float z = me->GetPositionZ();
                                    // me->UpdateGroundPositionZ(switchx, switchy, z);
                                    if (switchy > MIN_Y && switchy < MAX_Y)
                                    {
                                        me->GetMotionMaster()->MoveJump(switchx, switchy, z, 10, 10);
                                        me->AddDelayedEvent(3000, [this, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)] () -> void
                                        {
                                            if (aiLifetimeGuard.expired()) return;
                                            me->CastSpell(me, 229298);
                                            me->AddDelayedEvent(500, [this, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)] () -> void
                                            {
                                                if (aiLifetimeGuard.expired()) return;
                                                for (uint8 _up_down = 0; _up_down <= 1; ++_up_down)
                                                {
                                                    int8 _updown = (_up_down == 0 ? -1 : 1);
                                                    for (uint8 _length_x =1; _length_x <= 2; ++_length_x)
                                                    {
                                                        float _switchx = me->GetPositionX() + _updown*_length_x*SIZE_ONE_BOX;
                                                        if (_switchx > MIN_X && _switchx < MAX_X)
                                                        {
                                                            // for (uint8 _right_left = 0; _right_left <= 1; ++_right_left)
                                                            uint8 _right_left = urand(0, 1);
                                                            {
                                                                int8 _rightleft = (_right_left == 0 ? -1 : 1);
                                                                float _switchy = me->GetPositionY() + _rightleft*(3-_length_x)*SIZE_ONE_BOX;
                                                                float _z = me->GetPositionZ() + 2.0f;
                                                                // me->UpdateGroundPositionZ(_switchx, _switchy, _z);
                                                                if (_switchy > MIN_Y && _switchy < MAX_Y)
                                                                {
                                                                    me->CastSpell(_switchx, _switchy, _z, 229564);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            });
                                        });
                                        return;
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                case NPC_CHESS_ELEPHANT_DARK:
                case NPC_CHESS_ELEPHANT_HOLY:
                {
                    ///! Struct check:
                    // choose side for move by x
                    // choose side for move by y
                    // choose length
                    // if all ok, then jump + cast
                    //!
                    int8 randx = (urand(1,2) == 1 ? -1 : 1); //for rand during different time
                    int8 randy = (urand(1,2) == 1 ? -1 : 1); //for rand during different time
                    for (uint8 up_down = 0; up_down <= 1; ++up_down)
                    {
                        int8 updown = (up_down == 0 ? -1 : 1)*randx;
                        for (uint8 right_left = 0; right_left <= 1; ++right_left)
                        {
                            int8 rightleft = (right_left == 0 ? -1 : 1)*randy;
                            uint8 rand_length = urand(0, 4);
                            for (uint8 length = 6-rand_length; length >=0; --length)
                            {
                                float switchx = me->GetPositionX() + updown*length*SIZE_ONE_BOX;
                                float switchy = me->GetPositionY() + rightleft*length*SIZE_ONE_BOX;
  
                                if (switchy > MIN_Y && switchy < MAX_Y && switchx > MIN_X && switchx < MAX_X)
                                {
                                    float z = me->GetPositionZ();
                                    // me->UpdateGroundPositionZ(switchx, switchy, z);
                                    me->GetMotionMaster()->MoveJump(switchx, switchy, z, 10, 10);
                                    me->AddDelayedEvent(3000, [this, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)] () -> void
                                    {
                                        if (aiLifetimeGuard.expired()) return;
                                        me->CastSpell(me, (me->GetEntry() == NPC_CHESS_ELEPHANT_DARK ? 229558 : 229544));
                                        me->AddDelayedEvent(500, [this, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)] () -> void
                                        {
                                            if (aiLifetimeGuard.expired()) return;
                                            for (uint8 _up_down = 0; _up_down <= 1; ++_up_down)
                                            {
                                                int8 _updown = (_up_down == 0 ? -1 : 1);
                                                for (uint8 _right_left = 0; _right_left <= 1; ++_right_left)
                                                {
                                                    int8 _rightleft = (_right_left == 0 ? -1 : 1);
                                                    for (uint8 _length = 0; _length < 6; ++_length)
                                                    {
                                                        float _switchx = me->GetPositionX() + _updown*_length*SIZE_ONE_BOX;
                                                        float _switchy = me->GetPositionY() + _rightleft*_length*SIZE_ONE_BOX;
                          
                                                        if (_switchy > MIN_Y && _switchy < MAX_Y && _switchx > MIN_X && _switchx < MAX_X)
                                                        {
                                                            float _z = me->GetPositionZ();
                                                            // me->UpdateGroundPositionZ(_switchx, _switchy, _z);
                                                            me->CastSpell(_switchx, _switchy, _z, (me->GetEntry() == NPC_CHESS_ELEPHANT_DARK ? 229560 : 229546));
                                                        }
                                                        else
                                                            break;
                                                    }
                                                }
                                            }
                                        });
                                    });
                                    return;
                                }
                                else
                                    break;
                                
                            }
                        }
                    }
                    break;
                }
                case NPC_CHESS_ROOK:
                {
                    if (_rookArrived || (_rookMoving && me->movespline->GetId() == _rookSplineId &&
                        !me->movespline->Finalized()) || me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                    _rookMoving = _rookArrived = _rookArmed = false;
                    int8 firstSign = urand(0, 1) ? 1 : -1;
                    uint8 firstAxis = uint8(urand(0, 1));
                    for (uint8 direction = 0; direction != 2; ++direction)
                    {
                        uint8 axis = (firstAxis + direction) % 2;
                        for (uint8 side = 0; side != 2; ++side)
                        {
                            int8 sign = side ? -firstSign : firstSign;
                            for (int8 length = int8(6 - urand(0, 4)); length > 0; --length)
                            {
                                float x = me->GetPositionX() + (axis ? 0.0f : sign * length * SIZE_ONE_BOX);
                                float y = me->GetPositionY() + (axis ? sign * length * SIZE_ONE_BOX : 0.0f);
                                if (x <= MIN_X || x >= MAX_X || y <= MIN_Y || y >= MAX_Y)
                                    continue;
                                _rookDestination.Relocate(x, y, me->GetPositionZ());
                                me->GetMotionMaster()->MoveJump(_rookDestination, 10.0f, 10.0f, 229329);
                                _rookSplineId = me->movespline->GetId();
                                _rookMoving = true;
                                return;
                            }
                        }
                    }
                    break;
                }
                case NPC_CHESS_QUEEN:
                {
                    ///! Struct check:
                    // like ROOK (to-do, like elephant too)
                    //!
                    int8 rand_move = (urand(1,2) == 1 ? -1 : 1); //for rand during different time
                    int8 rand_v_h = urand(0,1); //for rand during different time
                    for (uint8 vertical_horizont = 0; vertical_horizont <= 1; ++vertical_horizont)
                    {
                        uint8 verhoriz = (vertical_horizont+rand_v_h % 2 == 0);
                        for (uint8 sign_ = 0; sign_ <= 1; ++sign_)
                        {
                            int8 sign = (sign_ == 0 ? -1 : 1)*rand_move;
                            uint8 rand_length = urand(0, 4);
                            for (int8 length = 6-rand_length; length >=0; --length)
                            {
                                float switchx = me->GetPositionX() + verhoriz*sign*length*SIZE_ONE_BOX;
                                float switchy = me->GetPositionY() + (!verhoriz)*sign*length*SIZE_ONE_BOX;
  
                                if (switchy > MIN_Y && switchy < MAX_Y && switchx > MIN_X && switchx < MAX_X)
                                {
                                    float z = me->GetPositionZ();
                                    // me->UpdateGroundPositionZ(switchx, switchy, z);
                                    me->GetMotionMaster()->MoveJump(switchx, switchy, z, 10, 10);
                                    me->AddDelayedEvent(3000, [this, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)] () -> void
                                    {
                                        if (aiLifetimeGuard.expired()) return;
                                        me->AddDelayedEvent(500, [this, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)] () -> void
                                        {
                                            if (aiLifetimeGuard.expired()) return;
                                            // like rook
                                            for (uint8 _vertical_horizont = 0; _vertical_horizont <= 1; ++_vertical_horizont)
                                            {
                                                for (uint8 _sign_ = 0; _sign_ <= 1; ++_sign_)
                                                {
                                                    int8 _sign = (_sign_ == 0 ? -1 : 1);
                                                    for (uint8 _length = 0; _length <= 3; ++_length)
                                                    {
                                                        float _switchx = me->GetPositionX() + _vertical_horizont*_sign*_length*SIZE_ONE_BOX;
                                                        float _switchy = me->GetPositionY() + (!_vertical_horizont)*_sign*_length*SIZE_ONE_BOX;
                          
                                                        if (_switchy > MIN_Y && _switchy < MAX_Y && _switchx > MIN_X && _switchx < MAX_X)
                                                        {
                                                            float _z = me->GetPositionZ();
                                                            // me->UpdateGroundPositionZ(_switchx, _switchy, _z);
                                                            me->CastSpell(_switchx, _switchy, _z, 229392);
                                                        }
                                                        else
                                                            break;
                                                    }
                                                }
                                            }
                                            // like elephant
                                            for (uint8 _up_down = 0; _up_down <= 1; ++_up_down)
                                            {
                                                int8 _updown = (_up_down == 0 ? -1 : 1);
                                                for (uint8 _right_left = 0; _right_left <= 1; ++_right_left)
                                                {
                                                    int8 _rightleft = (_right_left == 0 ? -1 : 1);
                                                    for (uint8 _length = 0; _length < 5; ++_length)
                                                    {
                                                        float _switchx = me->GetPositionX() + _updown*_length*SIZE_ONE_BOX;
                                                        float _switchy = me->GetPositionY() + _rightleft*_length*SIZE_ONE_BOX;
                          
                                                        if (_switchy > MIN_Y && _switchy < MAX_Y && _switchx > MIN_X && _switchx < MAX_X)
                                                        {
                                                            float _z = me->GetPositionZ();
                                                            // me->UpdateGroundPositionZ(_switchx, _switchy, _z);
                                                            me->CastSpell(_switchx, _switchy, _z, 229392);
                                                        }
                                                        else
                                                            break;
                                                    }
                                                }
                                            }
                                        });
                                    });
                                    return;
                                }
                                
                            }
                            
                        }
                    }
                    
                    break;
                }
            }
        }


    private:
        bool _rookMoving = false, _rookArrived = false, _rookArmed = false;
        uint32 _rookSplineId = 0;
        Position _rookDestination;
        // Unit-owned queues may outlive this AI object.
        std::shared_ptr<uint8> _delayedAILifetimeGuard = std::make_shared<uint8>(0);
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_karazhan_chessAI(creature);
    }
};

// 115388
class npc_karazhan_chess_king : public CreatureScript
{
public:
    npc_karazhan_chess_king() : CreatureScript("npc_karazhan_chess_king") {}

    struct npc_karazhan_chess_kingAI : public Scripted_NoMovementAI
    {
        npc_karazhan_chess_kingAI(Creature* creature) : Scripted_NoMovementAI(creature) 
        {
            me->setRegeneratingHealth(false);
            me->SetReactState(REACT_PASSIVE);
            SetCombatMovement(false);
        }

        EventMap events;
        
        void Reset() override
        {
            me->setRegeneratingHealth(false);
            me->SetReactState(REACT_PASSIVE);
            SetCombatMovement(false);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            me->CastSpell(me, 229572);
            me->CastSpell(me, 229470);
            // events.RescheduleEvent(EVENT_1, 13000);
        }
        
        void JustDied(Unit* who) override
        {
            me->CastSpell(me, 229566);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;
            
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        for (int8 posx = -1; posx <= 1; ++posx)
                            for (int8 posy = -1; posx <= 1; ++posy)
                            {
                                float _switchx = me->GetPositionX() + posx*SIZE_ONE_BOX;
                                float _switchy = me->GetPositionY() + posy*SIZE_ONE_BOX;

                                if (_switchy > MIN_Y && _switchy < MAX_Y && _switchx > MIN_X && _switchx < MAX_X)
                                {
                                    float _z = me->GetPositionZ() + 2.0f;
                                    // me->UpdateGroundPositionZ(_switchx, _switchy, _z);
                                    me->CastSpell(_switchx, _switchy, _z, 229426);
                                }
                            }
                        DoCast(229429);
                        events.RescheduleEvent(EVENT_1, 13000);
                        break;

                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_karazhan_chess_kingAI(creature);
    }
};

// 115496
class npc_karazhan_kadghar_intro : public CreatureScript
{
public:
    npc_karazhan_kadghar_intro() : CreatureScript("npc_karazhan_kadghar_intro") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_karazhan_kadghar_introAI(creature);
    }

    struct npc_karazhan_kadghar_introAI : public ScriptedAI
    {
        npc_karazhan_kadghar_introAI(Creature* creature) : ScriptedAI(creature) 
        {
            IntroDone = false;
        }
        
        bool IntroDone;

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;
            
            if (!IntroDone && me->IsWithinDistInMap(who, 10.0f))
            {
                IntroDone = true;
                Talk(0);
                me->AddDelayedEvent(5000, [actor = me] () -> void
                {
                    { if (auto ai = actor->AI()) ai->Talk(1); }
                    actor->DespawnOrUnsummon(5000);
                });
            }

        }
    };
};

void AddSC_karazhan_chess()
{
    new npc_karazhan_chess();
    new npc_karazhan_chess_king();
    new npc_karazhan_kadghar_intro();
};
