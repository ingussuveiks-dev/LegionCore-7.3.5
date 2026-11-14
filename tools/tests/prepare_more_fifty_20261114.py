"""Export production handlers and explicitly bounded numeric admission prefixes."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/MoreFifty20261114Repairs.json').read_text(encoding='utf-8'))


def block(source,start):
    a=source.index(start);b=source.index('{',a)+1;depth=1
    while depth:
        depth+=(source[b]=='{')-(source[b]=='}');b+=1
    return source[a:b]


def verify():
    for path,record in PLAN['files'].items():
        current=(ROOT/path).read_text(encoding='utf-8-sig')
        original='' if record.get('added') else subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
        assert hashlib.sha256(current.encode()).hexdigest()==record['after_sha256'],('Undeclared source change',path)
        assert hashlib.sha256(original.encode()).hexdigest()==record['before_sha256']
        for change in reversed(PLAN['changes']+PLAN['extra_edits']):
            if change['path']==path:
                assert current.count(change['new'])==1
                current=current.replace(change['new'],change['old'],1)
        if not record.get('added'):assert current==original,('Other gameplay changed',path)


def prepare(out,counter_case="",batch=0):
    verify();out.mkdir(parents=True,exist_ok=True)
    selected=[];defines=[];scopes={}
    for index,row in enumerate(PLAN['changes']):
        if batch and index//10+1!=batch:continue
        if counter_case and row['case']!=counter_case:continue
        source=(ROOT/row['path']).read_text(encoding='utf-8-sig')
        code=row['old'] if counter_case else block(source,'    static bool '+row['method']+'(')
        scopes[row['case']]='Complete production handler'
        if row.get('test_boundary'):
            boundary=code.index(row['test_boundary'])
            code=code[:boundary]+'        ++handler->databaseReached; handler->observed = '+row.get('observe','0')+'; return true;\n    }'
            if row.get('test_signature'):
                code=row['test_signature']+'\n'+code[code.index('    {'):]
            scopes[row['case']]='Production numeric admission prefix, stops before '+row['test_boundary'].strip()
        selected.append('namespace Case'+row['case']+' {\nstruct Selected {'+code+'};\n}')
        defines.append(('COUNTERFACTUAL_'if counter_case else'REPAIR_')+row['case'].upper())
    from prepare_go_travel_20261114 import coordinates
    coordinates(out)
    joined='\n'.join(selected)
    headers='\n'.join((ROOT/p).read_text(encoding='utf-8-sig') for p in (
        'src/server/game/Miscellaneous/Language.h','src/server/game/Miscellaneous/SharedDefines.h',
        'src/common/Common.h','src/server/game/AI/EventMap.h','src/server/game/Movement/MotionMaster.h','src/server/game/Entities/Object/ObjectGuid.h','src/server/game/Entities/Item/Item.h','src/server/game/Entities/Unit/Unit.h','src/server/game/Entities/Unit/UnitDefines.h','src/server/game/DataStores/DBCEnums.h'))
    constants=[]
    for name in sorted(set(re.findall(r'\b(?:LANG_[A-Z_0-9]+|TYPEID_PLAYER|TYPEID_UNIT|SEC_CONSOLE|VISUAL_WAYPOINT|EVENT_1|UNIT_FLAG2_MIRROR_IMAGE|ITEM_CHANGED|UNIT_FLAG_PVP|CURRENCY_TYPE_HONOR_POINTS|LOCALE_enUS)\b',joined+(ROOT/'tools/tests/MoreFifty20261114Test.cpp').read_text()))):
        match=re.search(r'\b'+name+r'\s*=\s*(0x[0-9a-fA-F]+|\d+)\b',headers) or re.search(r'#define\s+'+name+r'\s+(\d+)\b',headers)
        assert match,('Missing native constant',name)
        constants.append('constexpr uint32 '+name+'='+match[1]+';')
    unit=(ROOT/'src/server/game/Entities/Unit/Unit.h').read_text(encoding='utf-8-sig')
    shared=(ROOT/'src/server/game/Miscellaneous/SharedDefines.h').read_text(encoding='utf-8-sig')
    player=(ROOT/'src/server/game/Entities/Player/Player.h').read_text(encoding='utf-8-sig')
    constants += [block(unit,'enum '+name)+';' for name in ('UnitMoveType','UpdateStatsMask','CombatRating','DeathState','UnitMods','UnitModifierType')]
    constants += [block((ROOT/p).read_text(),'enum '+name)+';' for p,name in [('src/server/game/World/World.h','WorldIntConfigs'),('src/server/game/Entities/Pet/PetDefines.h','PetType'),('src/server/game/Movement/MotionMaster.h','MovementGeneratorType'),('src/server/database/Database/Implementation/WorldDatabase.h','WorldDatabaseStatements')]]
    constants += [block((ROOT/'src/server/game/Entities/Item/Item.h').read_text(),'enum '+name)+';' for name in ('InventoryResult','BuyResult','SellResult')]
    constants += [block(shared,'enum MailResponseType')+';',block(shared,'enum MailResponseResult')+';',block((ROOT/'src/server/game/DataStores/DBCEnums.h').read_text(),'enum CriteriaTypes')+';',block((ROOT/'src/server/game/DataStores/DBCEnums.h').read_text(),'enum Difficulty')+';']
    constants += [block(shared,'enum Powers')+';',block(shared,'enum Expansions')+';',block(player,'enum PlayerAvgItemLevelOffsets')+';',block((ROOT/'src/common/Common.h').read_text(),'enum TimeConstants')+';','constexpr uint8 CURRENT_EXPANSION=EXPANSION_LEGION;']
    constants += [block(shared,'enum WeatherType')+';',block(player,'enum TransferAbortReason')+';',block((ROOT/'src/server/game/World/World.h').read_text(),'enum ShutdownMask')+';',block((ROOT/'src/server/game/World/World.h').read_text(),'enum ShutdownExitCode')+';',block((ROOT/'src/server/game/World/World.h').read_text(),'enum WorldBoolConfigs')+';']
    constants.append(block((ROOT/'src/common/Common.h').read_text(),'enum LocaleConstant')+';\nconstexpr uint32 DEFAULT_LOCALE=LOCALE_enUS;')
    for name in ('INVENTORY_SLOT_BAG_0','PLAYER_EXPLORED_ZONES_SIZE'):
        match=re.search(r'\b'+name+r'\s*=\s*(0x[0-9a-fA-F]+|\d+)\b',headers+player) or re.search(r'#define\s+'+name+r'\s+(\d+)\b',headers+player)
        if match: constants.append('constexpr uint32 '+name+'='+match[1]+';')
        elif name=='CONFIG_WEATHER': constants.append('constexpr uint32 CONFIG_WEATHER=0; // Controlled bool-config key, not sent or persisted.')
        else: raise AssertionError(name)
    money=re.search(r'#define MAX_MONEY_AMOUNT\s+\(UI64LIT\((\d+)\)\)',player)
    assert money
    constants.append('constexpr uint64 MAX_MONEY_AMOUNT='+money[1]+'ULL;')
    constants.append((ROOT/'src/server/game/Entities/Object/Updates/UpdateFields.h').read_text(encoding='utf-8-sig'))
    constants.append(block((ROOT/'src/server/database/Database/Implementation/LoginDatabase.h').read_text(encoding='utf-8-sig'),'enum LoginDatabaseStatements')+';')
    native_money=block((ROOT/'src/server/game/Entities/Player/Player.cpp').read_text(encoding='utf-8-sig'),'bool Player::ModifyMoney(')
    native_money=native_money.replace('bool Player::ModifyMoney(','bool Unit::ModifyMoney(',1)
    rating=block((ROOT/'src/server/game/Entities/Player/Player.cpp').read_text(encoding='utf-8-sig'),'void Player::ApplyRatingMod(')
    (out/'NativeRating.inc').write_text(rating,encoding='utf-8')
    stat=(ROOT/'src/server/game/Entities/Unit/StatSystem.cpp').read_text(encoding='utf-8-sig')
    (out/'NativeSpellPower.inc').write_text(block(stat,'inline bool _ModifyUInt32(')+'\n'+block(stat,'void Player::ApplySpellPowerBonus('),encoding='utf-8')
    (out/'NativeModifyMoney.inc').write_text(native_money,encoding='utf-8')
    tokenizer=block((ROOT/'src/common/Utilities/Util.h').read_text(),'class TC_COMMON_API Tokenizer').replace('TC_COMMON_API ','')+';\n'+block((ROOT/'src/common/Utilities/Util.cpp').read_text(),'Tokenizer::Tokenizer(')
    (out/'NativeTokenizer.inc').write_text(tokenizer,encoding='utf-8')
    movement=(ROOT/'src/server/game/Movement/Spline/MovementUtil.cpp').read_text();typedefs=(ROOT/'src/server/game/Movement/Spline/MovementTypedefs.h').read_text()
    gravity=re.search(r'float constexpr gravity = [^;]+;',typedefs)[0];constantsFall='\n'.join(re.findall(r'    float constexpr [^;]+;',movement))
    (out/'NativeFall.inc').write_text('namespace Movement {\n'+gravity+'\n'+constantsFall+'\n'+block(movement,'    float computeFallElevation(')+'\n}\n',encoding='utf-8')
    guid=block((ROOT/'src/server/game/Entities/Object/ObjectGuid.cpp').read_text(),'ObjectGuid::LowType ObjectGuid::GetMaxCounter(HighGuid)')
    (out/'NativeGuidLimit.inc').write_text('#define UI64LIT(N) UINT64_C(N)\n'+guid+'\n#undef UI64LIT\n',encoding='utf-8')
    (out/'Selected.inc').write_text(joined,encoding='utf-8')
    (out/'Constants.inc').write_text('\n'.join(constants),encoding='utf-8')
    (out/'defines.txt').write_text(' '.join('/D'+d for d in defines),encoding='utf-8')
    (out/'source-inputs.json').write_text(json.dumps({'files':PLAN['files'],'counterfactual':counter_case,'batch':batch,'scopes':scopes,'services':'Controlled;no live account/character/world writes'},indent=2),encoding='utf-8')
    print('PASS exact source delta and native constant export;counterfactual='+str(bool(counter_case)))


if __name__=='__main__':
    if len(sys.argv)==1:
        verify();print('PASS exact authorized new source scope')
    else:
        mode=sys.argv[2]if len(sys.argv)>2 else''
        prepare(Path(sys.argv[1]),counter_case=mode[4:] if mode.startswith('old:') else '',batch=int(mode[6:]) if mode.startswith('batch:') else 0)
