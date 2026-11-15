"""Pin complete helper bodies and reverse all affected source files exactly."""
from pathlib import Path
import subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
BASE='aba0c7235'
EDITS={
'src/server/game/AI/ScriptedAI/ScriptedCreature.cpp':[
('''[this, entry, pos, spawnType, despawnTime]() -> void
    {
        if (me)
            me->SummonCreature(entry, pos, spawnType, despawnTime);''','''[actor = me, entry, pos, spawnType, despawnTime]() -> void
    {
        if (actor)
            actor->SummonCreature(entry, pos, spawnType, despawnTime);'''),
('''[=, this]() -> void
            {
                if (instance)
                    instance->RepopPlayersAtGraveyard();''','''[rejectedInstance = instance]() -> void
            {
                if (rejectedInstance)
                    rejectedInstance->RepopPlayersAtGraveyard();''')],
'src/server/game/AI/SmartScripts/SmartScript.cpp':[
('[this, saveState]() -> void { if (me) me->SetReactState(saveState); }','[actor = me, saveState]() -> void { if (actor) actor->SetReactState(saveState); }')],
'src/server/game/AI/CoreAI/PassiveAI.cpp':[
('''[this]
    {
        auto movementType = me->GetDefaultMovementType();
        if (movementType < MAX_DB_MOTION_TYPE && movementType != WAYPOINT_MOTION_TYPE)
            me->GetMotionMaster()->MoveRandom(urand(30, 50));''','''[actor = me]
    {
        auto movementType = actor->GetDefaultMovementType();
        if (movementType < MAX_DB_MOTION_TYPE && movementType != WAYPOINT_MOTION_TYPE)
            actor->GetMotionMaster()->MoveRandom(urand(30, 50));''')]}

def block(source,marker):
    a=source.index(marker);i=source.index('{',a);j=i+1;depth=1
    while depth:depth+=(source[j]=='{')-(source[j]=='}');j+=1
    return source[a:j]

def verify():
    originals={}
    for path,edits in EDITS.items():
        old=subprocess.check_output(['git','-C',str(ROOT),'show',BASE+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
        new=(ROOT/path).read_text(encoding='utf-8-sig');expected=old
        for before,after in edits:
            assert expected.count(before)==1,(path,'ambiguous original registration')
            expected=expected.replace(before,after,1)
        assert expected==new,(path,'undeclared source/payload change')
        originals[path]=old
    return originals

def generate(out,baseline=False):
    originals=verify();out=Path(out);out.mkdir(parents=True,exist_ok=True)
    for key,path,marker in [
        ('Summon','src/server/game/AI/ScriptedAI/ScriptedCreature.cpp','void ScriptedAI::SummonCreatureDelay('),
        ('Boss','src/server/game/AI/ScriptedAI/ScriptedCreature.cpp','void BossAI::_EnterCombat()'),
        ('Critter','src/server/game/AI/CoreAI/PassiveAI.cpp','void CritterAI::InitializeAI()'),
        ('React','src/server/game/AI/SmartScripts/SmartScript.cpp','case SMART_ACTION_SET_REACT_STATE:')]:
        text=originals[path] if baseline else(ROOT/path).read_text(encoding='utf-8-sig')
        (out/(key+'.inc')).write_text(block(text,marker),encoding='utf-8')
    (out/'FunctionProcessorActual.inc').write_text((ROOT/'src/common/Utilities/FunctionProcessor.cpp').read_text(),encoding='utf-8')

if __name__=='__main__':
    generate(sys.argv[1],len(sys.argv)>2 and sys.argv[2]=='baseline')
    print('PASS exact four core delayed-owner scopes: complete three source files match pinned baseline plus captures/receivers only; original payloads,delays,conditions and nonfailure paths unchanged')
