"""Review static artifact-map actors and recipient-owned timed-list LINKs.

Candidates are deliberately broader than individual routes. Nothing here proves
runtime actor availability, source22 admission, C++ delegation or client behavior.
"""
import argparse
from collections import Counter, defaultdict, deque
import hashlib
import json
from pathlib import Path
import sys
sys.dont_write_bytecode = True
from artifact_smart_links import invalid_links


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--world', type=Path, required=True)
    p.add_argument('--actors', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--date', default='2026-10-29')
    a = p.parse_args()
    hashes = {}

    def read(root, name):
        path = root / (name + '.jsonl')
        hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        return [json.loads(s) for s in path.read_text(encoding='utf-8-sig').splitlines()]

    scope = json.loads((a.actors/'scope.json').read_text(encoding='utf-8-sig'))
    scripts = defaultdict(list)
    for r in read(a.world, 'smart_scripts'):
        scripts[r['source_type'],r['entryorguid']].append(r)
    templates = {source: {r['entry']: r for r in read(a.world, name)}
                 for source,name in ((0,'creature_template'),(1,'gameobject_template'))}
    spawns = {source: read(a.actors,name) for source,name in ((0,'creature'),(1,'gameobject'))}
    by_guid = {source: {r['guid']:r for r in rows} for source,rows in spawns.items()}
    contexts, entries = {}, defaultdict(set)
    queue, seen = deque(), set()
    findings, unresolved, nested = [], [], []

    def context(source, entry, guid=0, reason='static'):
        # Native GetScript first tries -DBTableGUIDLow, falling back to template.
        script_entry = -guid if guid and scripts[source,-guid] else entry
        key = source,entry,script_entry
        if key not in contexts:
            t = templates[source].get(entry,{})
            execution = ('cpp_delegation_review' if t.get('ScriptName') else
                         'native_smart' if t.get('AIName') == ('SmartAI' if source == 0 else 'SmartGameObjectAI') else
                         'non_smart_or_missing_template')
            contexts[key] = dict(source_type=source,entry=entry,script_entry=script_entry,
                                 ai=t.get('AIName'),script=t.get('ScriptName'),execution=execution,
                                 maps=set(),spawn_count=0,reasons=set())
            entries[source,entry].add(key)
            queue.append((key,None))
        contexts[key]['reasons'].add(reason)
        return key

    for source,rows in spawns.items():
        for r in rows:
            c = contexts[context(source,r['id'],r['guid'])]
            c['maps'].add(r['map'])
            c['spawn_count'] += 1
    for source,field in ((0,'DirectCreatureEntries'),(1,'DirectGameObjectEntries')):
        for entry in scope[field]:
            context(source,entry,reason='route_template_candidate')

    def recipients(owner, row):
        target, param, param2 = (row[k] for k in ('target_type','target_param1','target_param2'))
        if target == 1:
            return [owner]
        source = 0 if target in (9,10,11,19) else 1 if target in (13,14,15,20) else None
        if source is None:
            return None
        if target in (10,14):
            spawn = by_guid[source].get(param)
            if not spawn or (param2 and spawn['id'] != param2):
                return None
            return [context(source,spawn['id'],spawn['guid'],reason='explicit_guid_recipient')]
        if not param:  # entry0 means any; never assume caller owns the new list.
            return None
        # Exact entry is a candidate, including GUID overrides; spatial/phase
        # eligibility is unknown. A summoned recipient can use its template.
        context(source,param,reason='explicit_entry_recipient_candidate')
        return sorted(entries[source,param])

    while queue:
        owner, timed = queue.popleft()
        if (owner,timed) in seen:
            continue
        seen.add((owner,timed))
        source,entry,script_entry = owner
        primary = scripts[source,script_entry]
        rows = scripts[9,timed] if timed is not None else primary
        c = contexts[owner]
        for f in invalid_links(rows, owner_events=primary):
            findings.append(dict(kind='invalid_timed_owner_link' if timed is not None else 'invalid_primary_link',
                                 owner_source=source,owner_entry=entry,owner_script_entry=script_entry,
                                 timed_list=timed,execution=c['execution'],maps=sorted(c['maps']),**f))
        for row in rows:
            action = row['action_type']
            origin = dict(owner_source=source,owner_entry=entry,owner_script_entry=script_entry,
                          timed_list=timed,event=row['id'],action=action,target_type=row['target_type'])
            if action == 12 and row['action_param1']:
                context(0,row['action_param1'],reason=f'action12_summon_candidate_from_{source}:{script_entry}:{timed}:{row["id"]}')
            if action not in (80,87,88):
                continue
            if action == 80:
                lists = [row['action_param1']]
            elif action == 87:
                lists = [row['action_param'+str(i)] for i in range(1,7) if row['action_param'+str(i)]]
                if not lists:
                    findings.append(dict(kind='empty_random_timed_pool',execution=c['execution'],**origin))
                    continue
            else:
                lo,hi = row['action_param1'],row['action_param2']
                if hi < lo or hi-lo > 256:
                    unresolved.append(dict(reason='invalid_or_unexpanded_range',min=lo,max=hi,**origin))
                    continue
                lists = range(lo,hi+1)
            targets = recipients(owner,row)
            if not targets:
                unresolved.append(dict(reason='target_none' if row['target_type']==0 else 'recipient_runtime_resolution_required',**origin))
                continue
            for recipient in targets:
                if timed is not None and recipient == owner:
                    nested.append(dict(reason='native_SetScript9_rejects_self_replacement_during_timed_iteration',**origin))
                    continue
                for list_id in sorted(set(lists)):
                    if not list_id:  # SetScript9(0) may deliberately clear the list.
                        continue
                    if not scripts[9,list_id]:
                        findings.append(dict(kind='missing_timed_action_list',list_id=list_id,
                                             recipient=list(recipient),execution=contexts[recipient]['execution'],**origin))
                    else:
                        queue.append((recipient,list_id))
    serial = [{**c,'maps':sorted(c['maps']),'reasons':sorted(c['reasons'])}
              for key,c in sorted(contexts.items())]
    result = dict(date=a.date,build='7.3.5.26972',scope_maps=scope['Maps'],
                  static_spawns={str(k):len(v) for k,v in spawns.items()},
                  context_counts=dict(Counter(c['execution'] for c in serial)),
                  timed_owner_contexts=sum(t is not None for _,t in seen),
                  contexts=serial,review_findings=findings,
                  unresolved_recipients=unresolved,nested_timed_replacement_review=nested,hashes=hashes,
                  limits=[f'All36 selection scope plus{len(scope["Maps"])} maps is a candidate inventory, not36 complete chains.',
                          'SmartScript::FindLinkedEvent uses recipient mEvents for source9 too. Ten extracted native cases verify the link evaluator; recipient selection is a static candidate expansion.',
                          'Flags/difficulty/source22 conditions/phase/distance and runtime event reachability are not evaluated; flagged rows require native behavioral review before SQL edits.',
                          'C++ bindings may override/delegate SmartAI; dormant rows are labeled, not asserted executable.',
                          'Direct action12 summon and explicit entry recipient closure are included; DB2 spell summons, C++ summons/SetScript9 and script base overrides/stored runtime targets are not fully traversed.',
                          'No missing LINK deleted and no quest/character/live record modified.'])
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('Artifact map review:',len(scope['Maps']),'maps;',len(serial),'owner contexts;',result['timed_owner_contexts'],'timed owner contexts')
    print('Review findings by kind:',dict(Counter(f['kind'] for f in findings)))
    print('Execution categories:',result['context_counts'],'; unresolved recipients:',len(unresolved),'; nested self replacement candidates:',len(nested))


if __name__ == '__main__':
    main()
