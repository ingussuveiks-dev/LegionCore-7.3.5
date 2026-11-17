#define main WardstoneRegressionMain
#include "SmartLinkedCastRetryTest.cpp"
#undef main
int main(int argc,char**argv)
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    auto rows=Read(argv[1]);assert(rows.size()==3);
    assert(rows[0].entryOrGuid==99615&&rows[0].event_id==19&&rows[0].event.type==SMART_EVENT_LINK&&rows[0].link==20&&rows[0].action.cast.spell==195739&&rows[0].action.cast.flags==512&&rows[0].targetType==1);
    assert(rows[1].event_id==20&&rows[1].actionType==1&&rows[1].parameter==6&&rows[1].link==21&&rows[1].action.cast.flags==7000);
    assert(rows[2].event_id==21&&rows[2].actionType==45&&rows[2].parameter==1&&!rows[2].link&&rows[2].targetType==19);
    assert(Validate(rows[0]));uint32 cases=0;Unit invoker;invoker.guid=2;
    auto receipt=[](SmartScript const&s){return s.mCompletedLinkedCasts.count(std::make_tuple(s.me->GetGUID(),int64(99615),uint32(19)));};
    for(uint32 failures:{0u,1u,2u,9u})for(bool missingOwner:{false,true}){
        Unit boss;auto s=Setup(boss,rows);boss.reject[195739]=failures;auto event=rows[0];s.ProcessEvent(event,&invoker);
        if(failures){assert(s.phaseTail.empty()&&!receipt(s));s.UpdateLinkedCastRetries(499);assert(s.phaseTail.empty());if(!missingOwner)units[2]=&invoker;s.UpdateLinkedCastRetries(1);}
        for(uint32 i=1;i<failures;++i)s.UpdateLinkedCastRetries(500);
        assert((boss.accepted==std::vector<uint32>{195739}&&s.phaseTail==std::vector<std::pair<uint32,uint32>>({{1,6},{45,1}})&&receipt(s)&&!s.credit&&!s.baseRequests&&!errors&&!warnings));
        auto duplicate=rows[0];s.ProcessEvent(duplicate,&invoker);assert(boss.accepted.size()==1&&s.phaseTail.size()==2);++cases;
    }
    for(uint32 lost=0;lost<5;++lost){Unit boss;auto s=Setup(boss,rows);boss.reject[195739]=1;auto event=rows[0];s.ProcessEvent(event,&invoker);if(lost==0)boss.alive=false;if(lost==1)boss.world=false;if(lost==2)boss.expired=true;if(lost==3)boss.guid=99;if(lost==4)s.OnReset();s.UpdateLinkedCastRetries(500);assert(s.phaseTail.empty()&&!receipt(s)&&s.mLinkedCastRetries.empty()&&!errors&&!warnings);++cases;}
    {Unit boss;auto s=Setup(boss,rows);boss.reject[195739]=100;auto event=rows[0];s.ProcessEvent(event,&invoker);for(uint32 i=0;i<10;++i)s.UpdateLinkedCastRetries(500);assert(warnings==1&&!errors&&!receipt(s)&&s.phaseTail.empty()&&s.mLinkedCastRetries.empty());std::puts("COUNTERFACTUAL expected: actual Awaken195739 failed after ten native retries; no speech/Gorewing handoff or receipt");++cases;}
    {Unit boss;auto s=Setup(boss,rows);bool reset=false;boss.onCast=[&](uint32){if(!reset){reset=true;s.OnReset();}};auto event=rows[0];s.ProcessEvent(event,&invoker);assert(!receipt(s)&&s.phaseTail.empty()&&s.mLinkedCastRetries.empty());boss.onCast={};event=rows[0];s.ProcessEvent(event,&invoker);assert(receipt(s)&&s.phaseTail.size()==2&&boss.accepted.size()==2&&!errors&&!warnings);++cases;}
    {Unit boss;auto s=Setup(boss,rows);bool nested=false;boss.onCast=[&](uint32){if(!nested){nested=true;auto e=rows[0];s.ProcessEvent(e,&invoker);}};auto event=rows[0];s.ProcessEvent(event,&invoker);assert(boss.accepted.size()==1&&s.phaseTail.size()==2&&receipt(s)&&s.mLinkedCastAttempts.empty()&&!errors&&!warnings);++cases;}
    std::printf("PASS %u actual195739/19 native512 CAST/LINK/receipt cases: acknowledgement before original TALK6/7000ms and Gorewing DATA1, independent rejection/busy/owner loss, actor death/expiry/replacement, reset/re-entry, exactly once and finite/reentrant failure. Spell/speech/data endpoints controlled; full effects/client separate.\n",cases);
}
