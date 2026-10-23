"""Keep all 36 acquisition routes and their outstanding evidence in one worklist."""
import argparse
import csv
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--routes', type=Path, required=True)
    parser.add_argument('--equip', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Refusing to overwrite an existing worklist and its accumulated review evidence')
    audit = json.loads(args.routes.read_text(encoding='utf-8-sig'))
    with args.equip.open(encoding='utf-8-sig') as stream:
        effects = [tuple(map(int, row)) for row in csv.reader(stream, delimiter='\t')]
    work = []
    for route in audit['routes']:
        work.append(dict(
            class_id=route['class_id'], specialization=route['specialization'],
            weapon=route['name'], item=route['item'], start=route['start'],
            status='queued for executable lifecycle and visual review; client traversal pending',
            quests=[dict(id=quest['id'], title=quest['title'],
                         prerequisites_any=quest['previous_any'],
                         lifecycle_review='pending', visual_review='pending')
                    for quest in route['quest_details']],
            pickup_objects=route['pickup_objects'],
            native_equip_effects=[dict(spell=spell, item=item, target1=target1, target2=target2)
                                  for spell, item, target1, target2, artifact in effects if item == route['item']],
            broken_actor_links=route['broken_actor_links'],
            video=dict(url=None, reviewed_intervals=[], status='not reviewed in this inventory'),
            checkpoints={name: 'pending' for name in (
                'quest_admission_and_faction_variants', 'actor_initial_pose_and_visibility',
                'stage_transitions_and_exactly_once_credit', 'player_pet_guardian_group_credit',
                'scaled_one_hit_surrender_or_death', 'npc_weapon_pickup_departure_visuals',
                'weapon_visibility_and_no_early_interaction', 'actual_loot_and_full_bags_retry',
                'equip_parent_and_paired_item', 'artifact_power_and_save_relog',
                'failed_cast_or_missing_actor_retry', 'abandon_reentry_cleanup',
                'camera_movement_and_pet_restoration', 'complete_client_traversal')},
            next_action='Trace the real quest/scenario and actor callbacks before changing data; compare pinned donors and then video where evidence is missing.'))
    assert len(work) == 36 and len({row['item'] for row in work}) == 36
    assert all(row['native_equip_effects'] for row in work)
    result = dict(
        date='2026-10-23', build='7.3.5.26972',
        scope='All 36 selected artifacts; this inventory certifies neither full chains nor visuals.',
        priority='Finish Hunter route review, then remaining classes; preserve previous Warlock repairs and user client observations in their original audits.',
        sources=dict(routes=str(args.routes.as_posix()),
                     route_sha256=hashlib.sha256(args.routes.read_bytes()).hexdigest(),
                     equip_sha256=hashlib.sha256(args.equip.read_bytes()).hexdigest(),
                     donor_review='docs/audits/legion-artifact-donor-review-2026-10-23.json'),
        common_findings=[dict(path='src/server/game/Spells/SpellEffects.cpp',
                              handler='EffectForceEquipItem', effect=249,
                              problem='Unconditional return disabled native equip for all 55 records.',
                              repair_audit='docs/audits/artifact-force-equip-2026-10-23.json',
                              limit='Each quest must still reach its equip spell after real acquisition; shared handler tests do not prove that callback or client visuals.')],
        routes=work)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print('Recorded 36 distinct artifact routes, native equip spells and explicit pending lifecycle/client checks.')


if __name__ == '__main__':
    main()
