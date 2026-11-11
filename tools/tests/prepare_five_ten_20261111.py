"""Extract actual repaired death/admission methods and numeric expressions.

External endpoints are controlled by the fixture. Native owner resolution,
matching DB2 data/bindings and the linked executable are separate checks.
"""
from pathlib import Path
import json,re,sys
import subprocess
from five_ten_20261111_scope import ROOT,PLAN,require_fixed

def mask(s):return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:re.sub(r'[^\n]',' ',m[0]),s,flags=re.S)

def block(s,pattern):
    masked=mask(s);m=re.search(pattern,masked);assert m,pattern
    start=masked.index('{',m.start());end=start+1;depth=1
    while depth:depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
    return s[m.start():end]

def generate(out,baseline=False,case=None):
    require_fixed([case]if case else None);out=Path(out);out.mkdir(parents=True,exist_ok=True)
    rows=[e for e in PLAN['changes']if e['kind']in ('death','admission')]
    methods=[e['edits'][0]['old' if baseline or e['edits'][0]['new']not in (ROOT/e['paths'][0]).read_text(encoding='utf-8-sig') else 'new']for e in rows]
    excluded={'NULL','AI','MS','TYPEID_PLAYER','TYPEID_UNIT','TEAM_ALLIANCE','TEAM_HORDE','QUEST_STATUS_INCOMPLETE','SIZE_OF_GRIDS','CAST_AI'}
    labels=sorted(set(re.findall(r'\b[A-Z][A-Z_0-9]+\b','\n'.join(methods)))-excluded-{'IN_MILLISECONDS'}|{'TalkDeath','GrandMarshalTremblade','AnnounceHordeKillBoss','AnnounceAllianceKillBoss','SpellAllianceReward','SpellHordeReward'})
    # Labels are controlled endpoint identifiers. Production keeps its real IDs;
    # native catalogs and PE checks independently verify those bindings/values.
    text='enum FixtureLabels{'+','.join(x+'='+str(i+100)for i,x in enumerate(labels))+'};\n'
    for i,method in enumerate(methods):text+='struct Frame'+str(i)+':Base{using Base::Base;'+method+'};\n'
    text+='#define CASE_DISPATCH '+''.join('case '+str(i)+':Run<Frame'+str(i)+'>(which);break;'for i in range(len(rows)))+'\n'
    text+='static int const nativeDeathIndex[] = {'+','.join(str(e.get('death_index',100+e.get('admission_index',0)))for e in rows)+'};\n'
    (out/'Selected.inc').write_text(text,encoding='utf8')
    numeric=[]
    for row in PLAN['changes']:
        if row['kind']!='numeric':continue
        numeric.append(row['key'])
    (out/'numeric-keys.json').write_text(json.dumps(numeric),encoding='utf8')
    mechanical_keys=['RhyolithPhaseHealth','MurmurResetHealth','ValithriaHealthComparisons','VanessaPhaseHealth','IchoronWaterHealth','ShaDoubtHealAmount','RogueCheatDeathHealth','DemonHunterResortHealth','HodirGazeHealth','GravityBombFatalHealth','XakalFissureInitialState','BroodlingBurstLifecycle','ShadowmasterMissingTorch','JohnnyPetRecovery','ThwackMissingDancer','FireworkPickerBounds','ThrowStaffBounds','RohnahDialogueOwnership','AntorusElevatorLifetime','VarimathrasTormentObjectLifetime','MeatballDeathQueueLifetime']
    cases=[dict(key=e['key'],index=i,mechanics=False)for i,e in enumerate(rows)]+[dict(key=key,index=i,mechanics=True)for i,key in enumerate(mechanical_keys)]
    assert len(cases)==50 and {e['key']for e in PLAN['changes']}=={c['key']for c in cases}
    (out/'cases.json').write_text(json.dumps(cases),encoding='utf8')
    mechanics(out,baseline)
    print('Extracted',len(rows),'complete production death/admission methods; controlled endpoints,not whole encounters')

def mechanics(out,baseline):
    def row(key):return next(e for e in PLAN['changes']if e['key']==key)
    def edit(key,n=0):
        e=row(key)['edits'][n]
        return e['old'if baseline or e['new']not in (ROOT/e['path']).read_text(encoding='utf-8-sig')else'new']
    def source(key):
        path=row(key)['paths'][0]
        return subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig')if baseline else(ROOT/path).read_text(encoding='utf-8-sig')
    def cls(key,name):return block(source(key),r'(?:class|struct)\s+'+name+r'\s*:[^{;]+\{')
    text=''
    for key,var in [('RhyolithPhaseHealth','_health'),('MurmurResetHealth','hp')]:
        text+='struct '+key+':Base{using Base::Base;uint64 Amount(){'+edit(key)+'return '+var+';}};\n'
    text+='struct Valithria:Base{using Base::Base;'+edit('ValithriaHealthComparisons')+'};\n'
    text+='struct Vanessa:Base{using Base::Base;bool Above(){return '+edit('VanessaPhaseHealth')+';}};\n'
    text+='struct Ichoron:Base{using Base::Base;'+block(cls('IchoronWaterHealth','boss_ichoronAI'),r'void\s+DoAction\s*\(')+'};\n'
    text+='struct Sha:Base{using Base::Base;void Heal(){Creature*creature=me;void*spellInfo=nullptr;'+edit('ShaDoubtHealAmount')+'}};\n'
    for key,name in [('RogueCheatDeathHealth','spell_rog_cheat_death_AuraScript'),('DemonHunterResortHealth','spell_dh_disable_absorb_AuraScript'),('HodirGazeHealth','spell_hodir_protective_gaze_AuraScript')]:
        text+='struct '+key+':Base{using Base::Base;'+block(cls(key,name),r'void\s+Absorb\s*\(')+'};\n'
    text+='struct Gravity:Base{using Base::Base;bool Periodic(float a){AuraEffect effect;effect.amount=a;AuraEffect*aurEff=&effect;Creature*owner=me;'+edit('GravityBombFatalHealth').replace('if (','return (',1)+' return true;return false;}bool Direct(int32 damage){hit=damage;'+edit('GravityBombFatalHealth',1).replace('if (','return (',1)+' return true;return false;}};\n'
    # Above conditions are expressions,not substitute implementations.
    text=text.replace(') return true;return false;',');')
    for key,name,field in [('XakalFissureInitialState','npc_xakal_fissureAI','active'),('BroodlingBurstLifecycle','npc_bethtilac_engorged_broodlingAI','bBurst')]:
        body=cls(key,name);decl=re.search(r'bool\s+'+field+r'[^;]*;',body)[0]
        text+='struct '+key+':Base{using Base::Base;'+decl
        if key.startswith('Xakal'):text+=block(body,r'void\s+Reset\s*\(')+block(body,r'void\s+OnAreaTriggerCast\s*\(')
        else:
            text+=block(body,r'void\s+'+('Reset'if 'bool bBurst = false;'in body else'EnterCombat')+r'\s*\(')+block(body,r'void\s+UpdateAI\s*\(')
        text+='};\n'
    for key,name,event in [('ShadowmasterMissingTorch','boss_brawguild_shadowmasterAI','EVENT_2'),('JohnnyPetRecovery','boss_brawguild_johnnyAI','EVENT_3'),('ThwackMissingDancer','npc_brawguild_thwack_u_controllerAI','EVENT_3')]:
        body=block(cls(key,name),r'void\s+UpdateAI\s*\(');m=re.search(r'case\s+'+event+r'\s*:',body);assert m
        masked=mask(body);depth=0;j=m.end()
        while j<len(body):
            if not depth and re.match(r'break\s*;',masked[j:]):j+=re.match(r'break\s*;',masked[j:]).end();break
            depth+=(masked[j]=='{')-(masked[j]=='}');j+=1
        selected=body[m.start():j]
        text+='struct '+key+':Base{using Base::Base;void Dispatch(){switch('+event+'){'+selected+'}}};\n'
    for key,array in [('FireworkPickerBounds','FireworksPicker'),('ThrowStaffBounds','RandomSpells')]:
        body=source(key);decl=block(body,r'uint32\s+'+array+r'\s*\[\s*\d+\s*\]\s*=\s*\{')+';'
        labels=set(re.findall(r'\bFIREWORK_[A-Z_0-9]+\b',decl))
        for label in sorted(labels):text+='static uint32 const '+label+'='+re.search(r'\b'+label+r'\s*=\s*(\d+)',body)[1]+';\n'
        text+=decl+'\nuint32 '+key+'(){return '+edit(key)+';}\n'
    # Both Rohnah registrations retain their full production dialogue payload.
    body=source('RohnahDialogueOwnership');registrations=[]
    for delay in ('3000','2500'):
        m=re.search(r'(?:me|Warlord)->AddDelayedEvent\(delay \+= '+delay+r', \[Warlord\]',body);assert m
        end=body.index('});',m.start())+3;registrations.append(body[m.start():end])
    text+='struct Rohnah:Base{using Base::Base;void Queue(Creature*Warlord){uint32 delay=0;'+registrations[0]+'delay=3500;'+registrations[1]+'}};\n'
    text+='struct Elevator:Base{using Base::Base;std::vector<int>mobsForElevator{1};void Queue(GameObject*go){'+edit('AntorusElevatorLifetime')+'}};\n'
    text+='struct Torment:Base{using Base::Base;void Queue(GameObject*go,Creature*varimathras){'+edit('VarimathrasTormentObjectLifetime')+'}};\n'
    # Capture/guard admission projection only;original payload is checked by
    # exact full-class reversal and linked closure dominance separately.
    cl=edit('MeatballDeathQueueLifetime');member=re.search(r'std::shared_ptr<uint8> _delayedAILifetimeGuard[^;]+;',cl)
    text+='struct Meatball:Base{using Base::Base;'+(member[0]if member else'')+'void Queue(uint32 which){if(which==0)me->AddDelayedEvent(1000,[this'+(', aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)'if member else'')+']{'+('if (aiLifetimeGuard.expired()) return;'if member else'')+'me->Touch();});else me->AddDelayedEvent(10000,[this'+(', aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)'if member else'')+']{'+('if (aiLifetimeGuard.expired()) return;'if member else'')+'me->Touch();});}};\n'
    (out/'Mechanics.inc').write_text(text,encoding='utf8')

if __name__=='__main__':generate(sys.argv[1],len(sys.argv)>2 and sys.argv[2]=='baseline',sys.argv[3]if len(sys.argv)>3 else None)
