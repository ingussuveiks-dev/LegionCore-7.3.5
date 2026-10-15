"""Audit all 36 native artifact selections against a read-only world snapshot.

This is structural coverage, not a client playthrough or a proof of every AI path.
"""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
from audit_spell_mechanics import Data

CLASSES = dict(WARRIOR=1,PALADIN=2,HUNTER=3,ROGUE=4,PRIEST=5,DEATH_KNIGHT=6,
               SHAMAN=7,MAGE=8,WARLOCK=9,MONK=10,DRUID=11,DEMON_HUNTER=12)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--world',type=Path,required=True)
    p.add_argument('--items',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    hashes={}
    def rows(name):
        path=args.world/(name+'.jsonl')
        hashes[name]=hashlib.sha256(path.read_bytes()).hexdigest()
        return [json.loads(s) for s in path.read_text(encoding='utf-8-sig').splitlines()]
    q={r['ID']:r for r in rows('quest_template')}
    for name in ('quest_template_addon','quest_template_locale'):
        for r in rows(name):
            if r['ID'] in q:q[r['ID']].update(r)
    objective=collections.defaultdict(list)
    for r in rows('quest_objectives'):objective[r['QuestID']].append(r)
    relations={n:collections.defaultdict(list) for n in ('creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender')}
    for n,d in relations.items():
        for r in rows(n):d[r['quest']].append(r['id'])
    source=Path('src/server/scripts/Legion/ClassHalls/class_hall_artifact_onboarding.cpp').read_text()
    routes=[dict(choice=int(a),response=int(b),class_id=CLASSES[c],chosen=int(d),start=int(e))
            for a,b,c,d,e in re.findall(r'\{\s*(\d+),\s*(\d+),\s*CLASS_(\w+),\s*(\d+),\s*(\d+)\s*\}',source)]
    assert len(routes)==36 and len({r['response'] for r in routes})==36
    catalog={int(r['ID']):r for r in csv.DictReader(args.items.open(encoding='utf-8-sig'),delimiter='\t')}
    products=rows('battlepay_product_item')
    shop={r['ItemID'] for r in products if r['ItemID'] in catalog}
    data=Data(Path('build-extractors/bin/Release/dbc/enUS'),Path('__native_only__'))
    effects=collections.defaultdict(list)
    for r in data.rows('SpellEffect').values():effects[r['SpellID']].append(r)
    pairs=data.rows('CharBaseInfo').values()
    native_pairs=sorted({(r['RaceID'],r['ClassID']) for r in pairs})
    native_lines=collections.defaultdict(list)
    by_line=collections.defaultdict(list)
    for r in data.rows('QuestLineXQuest').values():
        native_lines[r['QuestID']].append(r['QuestLineID']);by_line[r['QuestLineID']].append(r)
    steps=data.rows('ScenarioStep').values()
    lfg=data.rows('LfgDungeons').values()
    scenario_rows=rows('scenario_data')
    responses={(r['ChoiceId'],r['ResponseId']):r for r in rows('playerchoice_response')}
    rewards={(r['ChoiceId'],r['ResponseId']):r for r in rows('playerchoice_response_reward')}
    ct={r['entry']:r for r in rows('creature_template')}
    go={r['entry']:r for r in rows('gameobject_template')}
    loot=rows('gameobject_loot_template')
    scripts=rows('smart_scripts')
    conditions=rows('conditions')
    by_actor=collections.defaultdict(list)
    for r in scripts:by_actor[(r['entryorguid'],r['source_type'])].append(r)
    # Match the actual QuestData loader's OR prerequisites, including incoming
    # NextQuestID edges. PrevQuestID alone would incorrectly report shared finals.
    previous=collections.defaultdict(set)
    for r in q.values():
        if r.get('PrevQuestID'):previous[r['ID']].add(r['PrevQuestID'])
        if r.get('NextQuestID'):previous[abs(r['NextQuestID'])].add(r['ID'] if r['NextQuestID']>0 else -r['ID'])
    findings=[]
    for route in routes:
        title=q[route['chosen']].get('LogTitle','')
        def normalized(value):return re.sub(r'[^a-z0-9]','',value.lower().replace('the ','').replace('chosen','').replace('artifact','').strip())
        candidates=[i for i in shop if int(catalog[i]['ClassID'])==route['class_id'] and
                    (normalized(catalog[i]['Specialization']).startswith(normalized(title)) or
                     normalized(catalog[i]['Name']).startswith(normalized(title)))]
        assert len(candidates)==1,(route,title,candidates)
        item=candidates[0];route.update(item=item,name=catalog[item]['Name'],specialization=catalog[item]['Specialization'])
        chain=[];v=route['start']
        while v and v not in chain and len(chain)<32:
            if v not in q:
                findings.append(dict(kind='missing_link_target',quest=v,item=item));break
            chain.append(v);v=q[v].get('RewardNextQuest') or abs(q[v].get('NextQuestID',0))
        route['canonical_links']=chain
        line_rows=[r for line in native_lines[route['start']] for r in sorted(by_line[line],key=lambda r:r['OrderIndex'])]
        route['native_questline_sequence']=[dict(quest=r['QuestID'],order=r['OrderIndex'],line=r['QuestLineID']) for r in line_rows]
        scope=list(dict.fromkeys(chain+[r['QuestID'] for r in line_rows if r['QuestID'] in q]))
        route['link_cycle']=v if v in chain else None
        route['selection_row_present']=(route['choice'],route['response']) in responses
        reward=rewards.get((route['choice'],route['response']),{})
        route['selection_spell']=reward.get('SpellID',0)
        route['native_chosen_quest_effect']=any(e['Effect']==16 and e['EffectMiscValue1']==route['chosen'] for e in effects[route['selection_spell']])
        route['race_class_pairs']=[race for race,cls in native_pairs if cls==route['class_id']]
        route['quest_details']=[]
        actors=set()
        for quest in scope:
            r=q[quest]
            classes=r.get('AllowableClasses',0);races=r.get('AllowableRaces',0) or 0
            # Mirror Player::SatisfyQuestClass / SatisfyQuestRace. A zero race
            # mask is not unrestricted; uint64(-1) is the unrestricted value.
            eligible=[race for race in route['race_class_pairs'] if (not classes or (classes>0 and classes&(1<<(route['class_id']-1))) or (classes<0 and not(-classes&(1<<(route['class_id']-1))))) and (races==2**64-1 or races&(1<<(race-1)))]
            detail=dict(id=quest,title=r.get('LogTitle'),previous_any=sorted(previous[quest]),native_quest_lines=native_lines[quest],
                        class_mask=classes,race_mask=races,eligible_races=eligible,objectives=objective[quest],
                        accept_conditions=[c for c in conditions if c['SourceTypeOrReferenceId']==19 and c['SourceEntry']==quest],
                        starters={n:rel[quest] for n,rel in relations.items() if 'starter' in n},
                        enders={n:rel[quest] for n,rel in relations.items() if 'ender' in n})
            route['quest_details'].append(detail)
            if not eligible:findings.append(dict(kind='canonical_chain_class_race_conflict',item=item,quest=quest))
            for n,rel in relations.items():
                if n.startswith('creature'):actors.update(rel[quest])
            actors.update(o['ObjectID'] for o in objective[quest] if o['Type']==0)
        route['actors_with_cpp']=[dict(entry=a,script=ct[a]['ScriptName']) for a in sorted(actors) if a in ct and ct[a]['ScriptName']]
        broken=[]
        for a in actors:
            group=by_actor[(a,0)];ids={r['id'] for r in group}
            for r in group:
                if r['link'] and r['link'] not in ids:broken.append(dict(entry=a,event=r['id'],missing_link=r['link']))
        route['broken_actor_links']=broken
        scenario_actions=[]
        for a in scripts:
            if a['action_type']!=204:continue
            if a['event_type'] in (19,20) and a['event_param1'] not in scope:continue
            all_guards=[c for c in conditions if c['SourceTypeOrReferenceId']==22 and c['SourceEntry']==a['entryorguid'] and c['SourceGroup']==a['id']+1 and c['ConditionTypeOrReference'] in (8,9,41)]
            guards=[c for c in all_guards if c['ConditionValue1'] in scope]
            if all_guards and not guards:continue
            if a['entryorguid'] in actors or guards:
                scenario_actions.append(dict(entry=a['entryorguid'],event=a['id'],scenario=a['action_param1'],guards=guards,
                    native_routes=[dict(map=x['MapID'],difficulty=x['DifficultyID']) for x in lfg if x['ScenarioID']==a['action_param1']]))
        route['scenario_entry_actions']=scenario_actions
        scenario_ids={a['scenario'] for a in scenario_actions}
        risky_conditions=[]
        for c in conditions:
            if c['SourceTypeOrReferenceId']!=22 or c['ConditionTypeOrReference']!=44 or c['ConditionTarget']!=0 or c['ConditionValue1'] not in scenario_ids:continue
            event=next((a for a in by_actor[(c['SourceEntry'],c['SourceId'])] if a['id']==c['SourceGroup']-1),None)
            risky_conditions.append(dict(condition=c,event_type=event['event_type'] if event else None))
        route['scenario_conditions_using_unit_invoker']=risky_conditions
        entries={r['Entry'] for r in loot if r['Item']==item}
        pickups=[]
        for entry,g in go.items():
            if entry not in entries and not(g['type']==3 and g['Data1'] in entries):continue
            actions=by_actor[(entry,1)]
            spells={a['action_param1'] for a in actions if a['action_type'] in (11,85,86)}
            equips=[s for s in spells if any(e['Effect']==249 and e['EffectMiscValue1']==item for e in effects[s])]
            grants=[s for s in spells if any(e['Effect'] in (24,157,206) and e['EffectItemType']==item for e in effects[s])]
            rec=dict(entry=entry,type=g['type'],loot_link=g['Data1'],expected_loot_entries=sorted(entries),script=g['ScriptName'],
                     sai_spells=sorted(spells),native_equip_spells=equips,native_create_spells=grants)
            pickups.append(rec)
            if g['type']==3 and not g['Data1'] and not g['ScriptName'] and equips and not grants:
                findings.append(dict(kind='missing_artifact_chest_loot_link',entry=entry,item=item,pickup_spells=equips))
        route['pickup_objects']=pickups
        route['status']='structural audit; client traversal pending'
    review_links=collections.defaultdict(list)
    for route in routes:
        for broken in route['broken_actor_links']:
            review_links[(broken['entry'],broken['event'],broken['missing_link'])].append(route['item'])
    review_findings=[dict(kind='missing_actor_sai_link',entry=e,event=event,missing_link=link,
                          referenced_by_routes=sorted(set(items)),status='intended action unknown; needs executable route review')
                     for (e,event,link),items in sorted(review_links.items())]
    result=dict(date='2026-10-15',build='7.3.5.26972',routes=routes,findings=findings,review_findings=review_findings,
                native_race_class_pairs=native_pairs,scenario_routing=scenario_rows,
                native_artifact_related_scenarios=sorted({s['ScenarioID'] for s in steps if s['RewardQuestID'] in {q for r in routes for q in r['canonical_links']}}),
                native_lfg_scenarios=[dict(id=x['ID'],scenario=x['ScenarioID'],map=x['MapID'],difficulty=x['DifficultyID']) for x in lfg if x['ScenarioID']],
                hashes=dict(world=hashes,native=data.hashes,items=hashlib.sha256(args.items.read_bytes()).hexdigest(),item_sparse=hashlib.sha256(Path('build-extractors/bin/Release/dbc/enUS/ItemSparse.db2').read_bytes()).hexdigest(),
                    source={name:hashlib.sha256(Path(name).read_bytes()).hexdigest() for name in (
                        'src/server/scripts/Legion/ClassHalls/class_hall_artifact_onboarding.cpp',
                        'src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp',
                        'src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp',
                        'tools/research/ArtifactCatalog.cpp')}),
                limits=['Canonical Next/RewardNext links are not the complete execution graph. Native quest-line membership and OR prerequisites are recorded separately.',
                        'Race/class masks are structural admission checks; conditions, phasing, scripted callbacks, combat, abandonment, vehicles and visual playback require executable/client checks.',
                        'Missing object loot links exclude custom C++ handlers. Spell chain item grants, event callbacks and transient summoned actors still need review before changing a flagged row.',
                        'All 36 selections are enumerated; no route is certified end-to-end by this audit.'])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('Audited',len(routes),'artifact routes;',len(native_pairs),'native race/class pairs;',len(findings),'automated loot/link/admission findings;',len(review_findings),'actor links requiring review')
    for x in findings:print(json.dumps(x))

if __name__=='__main__':main()
