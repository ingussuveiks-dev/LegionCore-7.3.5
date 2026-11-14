"""Export declared production functions with explicit controlled-service boundaries."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
from prepare_more_fifty_20261114 import block
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/FifthFifty20261114Repairs.json').read_text(encoding='utf-8'))
def verify():
    for path,record in PLAN['files'].items():
        value=(ROOT/path).read_text(encoding='utf-8-sig')
        original=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
        assert hashlib.sha256(value.encode()).hexdigest()==record['after_sha256'],path
        assert hashlib.sha256(original.encode()).hexdigest()==record['before_sha256'],path
        for row in reversed(PLAN['changes']+PLAN['extra_edits']):
            if row['path']==path:
                assert value.count(row['new'])==1,(path,row.get('key'))
                value=value.replace(row['new'],row['old'],1)
        assert value==original,('Undeclared source delta',path)
def prepare(out,batch=0,counter=''):
    if counter and counter not in {'AppearanceMainhandExactItem','NpcPassengerFailedCreationCleanup','WaypointEventTypedMutation','TypedCommandArgumentsBoundsAndValues','LinkedWorldObjectExactCounter','DeletedCharacterRetentionExactDays'}:
        raise ValueError('No safe pinned counterfactual declared: '+counter)
    verify();out.mkdir(parents=True,exist_ok=True);selected=[];defines=[];scopes={}
    chat=(ROOT/'src/server/game/Chat/Chat.cpp').read_text(encoding='utf-8-sig')
    for i,row in enumerate(PLAN['changes']):
        if batch and i//10+1!=batch:continue
        if counter and counter!=row['case']:continue
        code=row['old']if counter else row['new'];key=row['case'];scopes[key]='Complete production function with controlled endpoints'
        defines.append('/D'+('COUNTERFACTUAL_'if counter else'REPAIR_')+key.upper())
        if row['kind'].startswith('core'):
            continue
        if row.get('test_boundary'):
            prefix=code[:code.index(row['test_boundary'])];depth=prefix.count('{')-prefix.count('}')
            code=prefix+'        ++handler->reached; handler->observed = '+row.get('observe','0')+'; '+row.get('test_capture','')+' return true;\n'+'}\n'*(depth-1)+('return false;\n'if depth>1 else'')+'}\n'
            if row.get('test_signature'):code=row['test_signature']+'\n'+code[code.index('    {'):]
            scopes[key]='Actual production admission prefix; stops before '+row['test_boundary'].strip()
        selected.append('namespace Case'+key+' {struct Selected {\n'+code+'\n};}\n')
    if counter:assert scopes,('Unknown counterfactual',counter)
    links=block(chat,'char* ChatHandler::extractKeyFromLink(char* text, char const* linkType')+'\n'+block(chat,'char* ChatHandler::extractKeyFromLink(char* text, char const* const* linkTypes')+'\n'+block(chat,'char* ChatHandler::extractQuotedArg(')+'\n'+block(chat,'void ChatHandler::extractOptFirstArg(')
    links+='\n'+block(chat,'enum SpellLinkType')+';\n'+re.search(r'static char const\* const spellKeys\[\] =\s*\{.*?\};',chat,re.S)[0]+'\n'+block(chat,'uint32 ChatHandler::extractSpellIdFromLink(')
    links+='\n'+block(chat,'enum GuidLinkType')+';\n'+re.search(r'static char const\* const guidKeys\[\] =\s*\{.*?\};',chat,re.S)[0]+'\n'
    for method,signature in [('extractGuidFromLink','ObjectGuid ChatHandler::extractGuidFromLink('),('extractGameTeleFromLink','GameTele const* ChatHandler::extractGameTeleFromLink('),('Initialize','void CommandArgs::Initialize(')]:
        row=next((r for r in PLAN['changes']if r['method']==method and r['kind'].startswith('core')),None)
        links+=(row['old']if row and counter==row['case']else block(chat,signature))+'\n'
    util=(ROOT/'src/common/Utilities/Util.cpp').read_text(encoding='utf-8-sig')
    links=block(util,'bool isNumeric(char c)')+'\n'+block(util,'bool isNumeric(char const* str)')+'\n'+links
    links=block((ROOT/'src/common/Utilities/Util.h').read_text(),'class TC_COMMON_API Tokenizer').replace('TC_COMMON_API ','')+';\n'+block(util,'Tokenizer::Tokenizer(')+'\n'+links
    joined='\n'.join(selected)+links
    headers='\n'.join((ROOT/p).read_text(encoding='utf-8-sig')for p in ['src/server/game/Miscellaneous/Language.h','src/server/game/Miscellaneous/SharedDefines.h','src/common/Common.h','src/server/game/Entities/Player/Player.h','src/server/game/Groups/Group.h','src/server/game/Chat/Chat.h','src/server/game/Movement/MotionMaster.h'])
    fixture=(ROOT/'tools/tests/FifthFifty20261114Test.cpp').read_text(encoding='utf-8')
    constants=[]
    for name in sorted(set(re.findall(r'\b(?:LANG_[A-Z_0-9]+|EQUIPMENT_SLOT_[A-Z_0-9]+|MAX_RAID_SIZE|VISUAL_WAYPOINT|SPELL_FAILED_UNKNOWN|CHEAT_[A-Z_]+|CHAR_NAME_SUCCESS)\b',joined+fixture+' '.join(re.findall(r'\bEQUIPMENT_SLOT_[A-Z_0-9]+',headers))))):
        match=re.search(r'\b'+name+r'\s*=\s*(0x[0-9a-fA-F]+|\d+)\b',headers)or re.search(r'#define\s+'+name+r'\s+(\d+)\b',headers)
        assert match,('Native constant missing',name)
        constants.append('constexpr uint32 '+name+'='+match[1]+';')
    for p,name in [('src/server/database/Database/Implementation/WorldDatabase.h','WorldDatabaseStatements'),('src/server/database/Database/Implementation/CharacterDatabase.h','CharacterDatabaseStatements'),('src/server/database/Database/Implementation/LoginDatabase.h','LoginDatabaseStatements')]:
        constants.append(block((ROOT/p).read_text(encoding='utf-8-sig'),'enum '+name)+';')
    # Configuration key is imported, never used against a live world.
    constants.append(block((ROOT/'src/server/game/World/World.h').read_text(encoding='utf-8-sig'),'enum WorldIntConfigs')+';')
    battleground=(ROOT/'src/server/game/Battlegrounds/BattlegroundDefines.h').read_text(encoding='utf-8-sig')
    bracket=block(battleground,'        namespace JoinType')+'\n'+block(battleground,'        namespace BracketType')+'\n'+block(battleground,'        static uint8 GetJoinTypeByBracketSlot(')
    (out/'NativeBracket.inc').write_text('namespace MS {namespace Battlegrounds {\n'+bracket+'\n}}',encoding='utf-8')
    constants.append(block((ROOT/'src/common/Common.h').read_text(),'enum TimeConstants')+';')
    cleanup=block((ROOT/'src/server/game/Globals/ObjectMgr.cpp').read_text(encoding='utf-8-sig'),'void ObjectMgr::DeleteCreatureData(')
    group=block((ROOT/'src/server/scripts/Commands/cs_debug.cpp').read_text(encoding='utf-8-sig'),'    static bool HandleDebugGroupCommand(')
    line=next(line for line in group.splitlines()if 'count 'in line and 'group->GetMembersCount()'in line)
    deletion=block((ROOT/'src/server/game/Entities/Player/Player.cpp').read_text(encoding='utf-8-sig'),'void Player::DeleteOldCharacters(uint32 keepDays)')
    if counter=='DeletedCharacterRetentionExactDays':
        deletion=next(row['old']for row in PLAN['extra_edits']if row['owner_key']==counter and row['path'].endswith('/Player.cpp'))
    (out/'NativeDeletion.inc').write_text(deletion,encoding='utf-8')
    (out/'NativeCleanupAndRemaining.inc').write_text(cleanup+'\nstatic uint8 NativeGroupRemaining(uint8 count,ControlledGroup*group) {\n'+line+'\nreturn count;}',encoding='utf-8')
    (out/'Constants.inc').write_text('\n'.join(constants),encoding='utf-8')
    (out/'NativeChatLinks.inc').write_text(links,encoding='utf-8')
    (out/'Selected.inc').write_text('\n'.join(selected),encoding='utf-8')
    (out/'defines.txt').write_text(' '.join(defines),encoding='utf-8')
    (out/'source-inputs.json').write_text(json.dumps(dict(files=PLAN['files'],batch=batch,counterfactual=counter,scopes=scopes,services='Controlled only;no live DB/account/character writes'),indent=2),encoding='utf-8')
    import runpy
    runpy.run_path(str(ROOT/'tools/tests/fifth_fifty_20261114_cases.py'))['generate'](out)
    print('PASS exact inverse source certificate and production export',len(scopes))
if __name__=='__main__':
    if len(sys.argv)==1:verify();print('PASS exact source scope')
    else:
        mode=sys.argv[2]if len(sys.argv)>2 else''
        prepare(Path(sys.argv[1]),int(mode[6:])if mode.startswith('batch:')else 0,mode[4:]if mode.startswith('old:')else'')
