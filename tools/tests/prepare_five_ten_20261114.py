"""Export actual selected code, with a declared pre-DB boundary for WP info."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/FiveTen20261114Repairs.json').read_text(encoding='utf-8'))


def block(source,start):
    a=source.index(start);b=source.index('{',a)+1;depth=1
    while depth:
        depth+=(source[b]=='{')-(source[b]=='}');b+=1
    return source[a:b]


def verify():
    for path,record in PLAN['files'].items():
        current=(ROOT/path).read_text(encoding='utf-8-sig')
        original=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
        assert hashlib.sha256(current.encode()).hexdigest()==record['after_sha256'],('Undeclared source change',path)
        assert hashlib.sha256(original.encode()).hexdigest()==record['before_sha256']
        for change in reversed(PLAN['changes']+PLAN['extra_edits']):
            if change['path']==path:
                assert current.count(change['new'])==1
                current=current.replace(change['new'],change['old'],1)
        assert current==original,('Other gameplay changed',path)


def prepare(out,baseline=False):
    verify();out.mkdir(parents=True,exist_ok=True)
    selected=[];defines=[]
    for row in PLAN['changes']:
        kind=row['kind']
        if kind=='account_realm' and not baseline:
            defines.append('REPAIR_ACCOUNT_REALM')
        if kind in ('go','account_realm') or baseline and kind!='soul':
            continue
        source=(ROOT/row['path']).read_text(encoding='utf-8-sig')
        code=block(source,'    static bool '+row['method']+'(') if kind!='soul' else row['old' if baseline else 'new']
        name={'waypoint':'Waypoint','account':'Account','cast':'Cast','titles':'Titles','soul':'Soul'}.get(kind)
        if kind=='power':name={'HandleModifyEnergyCommand':'Energy','HandleModifyRageCommand':'Rage','HandleModifyRunicPowerCommand':'Runic'}[row['method']]
        if kind=='waypoint':
            boundary=code.index('            WorldDatabasePreparedStatement* stmt =')
            code=code[:boundary]+'            ++handler->databaseReached; return true;\n        }\n        return false;\n    }'
        body=('struct Selected : EchoBase {' if kind=='soul' else 'struct Selected {')+code+'};'
        if kind=='soul':
            body=block(source,'enum Spells')+';'+block(source,'enum eAnim')+';'+re.search(r'uint32 AnimRand\[3\]\s*=\s*\{[^}]*\};',source)[0]+body
        selected.append('namespace Case'+name+' {\n'+body+'\n}')
        defines.append('REPAIR_'+name.upper())
    joined='\n'.join(selected)
    headers='\n'.join((ROOT/p).read_text(encoding='utf-8-sig') for p in (
        'src/server/game/Miscellaneous/Language.h','src/server/game/Miscellaneous/SharedDefines.h',
        'src/common/Common.h','src/server/game/AI/EventMap.h','src/server/game/Movement/MotionMaster.h'))
    constants=[]
    for name in sorted(set(re.findall(r'\b(?:LANG_[A-Z_0-9]+|POWER_ENERGY|POWER_RAGE|POWER_RUNIC_POWER|SEC_CONSOLE|VISUAL_WAYPOINT|EVENT_1)\b',joined+(ROOT/'tools/tests/FiveTen20261114Test.cpp').read_text()))):
        match=re.search(r'\b'+name+r'\s*=\s*(\d+)\b',headers) or re.search(r'#define\s+'+name+r'\s+(\d+)\b',headers)
        assert match,('Missing native constant',name)
        constants.append('constexpr uint32 '+name+'='+match[1]+';')
    constants.append((ROOT/'src/server/game/Entities/Object/Updates/UpdateFields.h').read_text(encoding='utf-8-sig'))
    constants.append(block((ROOT/'src/server/database/Database/Implementation/LoginDatabase.h').read_text(encoding='utf-8-sig'),'enum LoginDatabaseStatements')+';')
    (out/'Selected.inc').write_text(joined,encoding='utf-8')
    (out/'Constants.inc').write_text('\n'.join(constants),encoding='utf-8')
    (out/'defines.txt').write_text(' '.join('/D'+d for d in defines)+(' /DSOUL_RANGE_BASELINE' if baseline else ''),encoding='utf-8')
    (out/'source-inputs.json').write_text(json.dumps({'files':PLAN['files'],'baseline':baseline,'scope':'Actual handlers/summon callback;WP prefix stops before first DB query;services controlled'},indent=2),encoding='utf-8')
    print('PASS exact source delta and native constant export;baseline='+str(baseline))


if __name__=='__main__':
    if len(sys.argv)==1:
        verify();print('PASS exact first-ten source scope')
    else:
        prepare(Path(sys.argv[1]),len(sys.argv)>2 and sys.argv[2]=='baseline')
