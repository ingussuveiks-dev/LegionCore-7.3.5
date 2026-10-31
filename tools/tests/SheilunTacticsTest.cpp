#include "NativeHost.inc"

int main()
{
#ifdef BASELINE
    for(uint32 role=0;role<3;++role)for(uint32 choice=0;choice<2;++choice)
    {
        Run r;auto actor=r.Role(role);r.Click(actor,choice);
        assert(r.player.requests==1&&actor->tacticCasts==0&&actor->auras.empty());
    }
    std::cout<<"PASS EXPECTED BROKEN872b3da2a: all six real role/index selections credit their role without casting or confirming either native tactic. No client or ordinary-success claim.\n";
#else
    unsigned cases=0;
    for(uint32 role=0;role<3;++role)for(uint32 early=0;early<2;++early)
        for(uint32 choice=0;choice<2;++choice)for(uint32 prior=0;prior<4;++prior)
    {
        Run r;auto actor=r.Role(role);uint32 chosen=choice?204653:204654,opposite=choice?204654:204653;
        if(early)actor->spawn=role==0?362835:role==1?362834:362831;
        uint32 utility=role==0?203836:role==1?204593:204624;
        actor->auras.insert(utility);actor->auras.insert(999);
        if(prior&1)actor->auras.insert(chosen);if(prior&2)actor->auras.insert(opposite);
        r.player.onCredit=[&]{assert(actor->HasAura(chosen)&&!actor->HasAura(opposite)&&actor->HasAura(utility)&&actor->HasAura(999));};
        r.Click(actor,choice);assert(r.player.requests==1&&actor->tacticCasts==1&&actor->tacticRemoves==1&&!tacticWarnings&&!(actor->flags&1));
        r.Click(actor,1-choice);assert(r.player.requests==1&&actor->tacticCasts==1&&actor->HasAura(chosen));++cases;
    }
    for(uint32 role=0;role<3;++role)for(uint32 choice=0;choice<2;++choice)for(uint32 failure=0;failure<4;++failure)
    {
        Run r;auto actor=r.Role(role);uint32 chosen=choice?204653:204654,opposite=choice?204654:204653;actor->auras.insert(opposite);
        if(failure==0)actor->tacticResult=SPELL_FAILED_ERROR;
        if(failure==1)actor->tacticApplies=false;
        if(failure==2)actor->removeWorks=false;
        if(failure==3)actor->onTacticRemove=[&]{actor->auras.erase(chosen);};
        r.Click(actor,choice);assert(!r.player.requests&&(actor->flags&1)&&tacticWarnings==1);
        if(failure<2)assert(actor->HasAura(opposite)&&!actor->HasAura(chosen)&&!actor->tacticRemoves);
        actor->tacticResult=SPELL_CAST_OK;actor->tacticApplies=actor->removeWorks=true;actor->onTacticRemove={};
        r.Click(actor,choice);assert(r.player.requests==1&&actor->HasAura(chosen)&&!actor->HasAura(opposite)&&!(actor->flags&1)&&tacticWarnings==1);++cases;
    }
    for(uint32 role=0;role<3;++role)for(uint32 choice=0;choice<2;++choice)
    {
        Run r;auto actor=r.Role(role);r.player.accept=false;r.Click(actor,choice);
        assert(r.player.requests==1&&(actor->flags&1)&&actor->HasAura(choice?204653:204654));
        r.player.accept=true;r.Click(actor,1-choice);
        assert(r.player.requests==2&&actor->tacticCasts==2&&actor->HasAura(choice?204654:204653)&&!actor->HasAura(choice?204653:204654)&&!(actor->flags&1)&&!tacticWarnings);++cases;
    }
    for(uint32 role=0;role<3;++role)for(uint32 choice=0;choice<2;++choice)
        for(uint32 boundary=0;boundary<2;++boundary)for(uint32 revoke=0;revoke<16;++revoke)
    {
        Run r;auto actor=r.Role(role);auto callback=[&]{switch(revoke){
            case 0:r.player.quest.ScriptLifetime.reset();break;
            case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;
            case 2:r.player.map=nullptr;break;
            case 3:r.map.actors.erase(actor->guid.value);break;
            case 4:r.map.players.clear();break;
            case 5:r.player.status=QUEST_STATUS_NONE;break;
            case 6:actor->world=false;break;
            case 7:++actor->entry;break;
            case 8:++actor->spawn;break;
            case 9:actor->predelete=true;break;
            case 10:++r.scenario.id;break;
            case 11:r.scenario.step=3;break;
            case 12:r.player.phase=false;break;
            case 13:r.player.rewarded=true;break;
            case 14:actor->alive=false;break;
            case 15:r.player.range=false;break;
        }};
        if(boundary)actor->onTacticRemove=callback;else actor->onTacticCast=callback;
        r.Click(actor,choice);assert(!r.player.requests&&(actor->flags&1)&&!tacticWarnings);
        assert(actor->tacticRemoves==boundary);++cases;
    }
    for(uint32 role=0;role<3;++role)for(uint32 choice=0;choice<2;++choice)for(uint32 boundary=0;boundary<2;++boundary)
    {
        Run r;auto actor=r.Role(role);auto callback=[&]{r.Click(actor,1-choice,false);};
        if(boundary)actor->onTacticRemove=callback;else actor->onTacticCast=callback;
        r.Click(actor,choice);assert(r.player.requests==1&&actor->tacticCasts==1&&!(actor->flags&1)&&!tacticWarnings);++cases;
    }
    std::cout<<"PASS "<<cases<<" actual native tactic acknowledgement and full plan/packet/menu helpers under ASAN: six qualified sources, both options/all prior modes, aura-before-credit and exclusive counterpart-only cleanup, utility retention, rejected cast/accepted-without-aura/failed cleanup/lost aura retry, rejected native role event and changed-choice retry,192 cast/removal reentrant source/player/quest/scenario revocations, stale-menu reentry and duplicate denial. Cast/aura/world endpoints controlled; native damage data/handlers and client fidelity are separate checks.\n";
#endif
}
