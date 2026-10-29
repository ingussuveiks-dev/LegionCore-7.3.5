"""Exercise owner/GUID/recipient traversal against a small exported world."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]


def row(source, entry, event, *, link=0, event_type=0, action=1, params=(), target=1, target_param=0):
    result = dict(source_type=source,entryorguid=entry,id=event,link=link,event_type=event_type,
                  action_type=action,target_type=target,target_param1=target_param,target_param2=0)
    result.update({'action_param'+str(i+1):params[i] if i<len(params) else 0 for i in range(6)})
    return result


with tempfile.TemporaryDirectory(prefix='artifact-map-links-') as directory:
    base = Path(directory)
    world, actors = base/'world', base/'actors'
    world.mkdir(); actors.mkdir()

    def save(folder, name, rows):
        (folder/(name+'.jsonl')).write_text(''.join(json.dumps(r)+'\n' for r in rows),encoding='utf-8')

    (actors/'scope.json').write_text(json.dumps(dict(Maps=[1489],DirectCreatureEntries=[],DirectGameObjectEntries=[])))
    save(actors,'creature',[dict(guid=99,id=10,map=1489)])
    save(actors,'gameobject',[dict(guid=88,id=50,map=1489)])
    save(world,'creature_template',[dict(entry=i,AIName='SmartAI',ScriptName='') for i in (10,20,30)])
    save(world,'gameobject_template',[dict(entry=50,AIName='SmartGameObjectAI',ScriptName='')])
    save(world,'smart_scripts',[
        row(0,10,0,link=999),  # Unused template when -guid override exists.
        row(0,-99,0,action=80,params=(100,)),
        row(0,-99,1,event_type=61),
        row(9,100,2,link=1),
        row(9,100,1,event_type=64),  # Source9's own row1 must not be the LINK target.
        row(9,100,3,action=80,params=(101,),target=11,target_param=20),
        row(9,100,4,action=80,params=(999,)),  # Native timed self replacement refusal.
        row(0,20,1,event_type=64),
        row(9,101,2,link=1),  # Real recipient's row1 is non-LINK: reject.
        row(9,101,3,action=12,params=(30,)),
        row(9,101,4,action=88,params=(5,400)),  # Expansion must remain visible/unresolved.
        row(0,30,0,action=87),  # Empty pool diagnostic candidate.
        row(0,30,1,action=80,params=(777,),target=0),
        row(0,30,2,action=80,params=(778,),target=7),
        row(0,30,3,action=80,params=(0,)),  # Deliberate clear is not missing-list defect.
        row(0,30,4,action=80,params=(800,)),
        row(1,50,0,action=80,params=(300,)),
        row(1,50,1,event_type=61),
        row(9,300,2,link=1)
    ])
    result = subprocess.run([sys.executable,str(root/'tools/research/audit_artifact_map_links.py'),
                             '--world',str(world),'--actors',str(actors),'--output',str(base/'result.json')],
                            capture_output=True,text=True,timeout=30)
    assert result.returncode == 0 and not result.stderr, result.stdout+result.stderr
    report = json.loads((base/'result.json').read_text())
    findings = report['review_findings']
    assert len(findings) == 3, findings
    invalid = next(f for f in findings if f['kind']=='invalid_timed_owner_link')
    assert (invalid['owner_entry'],invalid['timed_list'],invalid['reason']) == (20,101,'invalid_target_event')
    assert next(f for f in findings if f['kind']=='empty_random_timed_pool')['owner_entry'] == 30
    assert next(f for f in findings if f['kind']=='missing_timed_action_list')['list_id'] == 800
    assert len(report['nested_timed_replacement_review']) == 1
    assert {f['reason'] for f in report['unresolved_recipients']} == {
        'invalid_or_unexpanded_range','target_none','recipient_runtime_resolution_required'}
    assert report['timed_owner_contexts'] == 3
    assert not any(c['entry']==10 and c['script_entry']==10 for c in report['contexts'])
    assert any(c['source_type']==1 and c['entry']==50 for c in report['contexts'])
print('PASS GUID override/fallback scope, recipient-owned source9 LINK resolution, cross-creature and GO dispatch, direct summon closure, native self-replacement refusal, empty/random/range/clear/missing lists and visible unresolved recipients. Ten native LINK cases separately verify evaluator semantics.')
