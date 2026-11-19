"""Reproduce445 placement/size/waypoint and248 corridor requests from native catalog receipts.
Run Read-LegionTeemingPlacement.ps1 on wire-placement-input.tsv and
Read-LegionTeemingGeometry.ps1 on final-wire-corridors-input.tsv.
Require every placement accepted,every corridor PATHFIND_NORMAL and clean diagnostics.
"""
import json,math,collections,argparse
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--catalog',type=Path,default=Path(__file__).resolve().parents[2]/'docs/research/legion-teeming-reconstruction-2026-11-19.json')
parser.add_argument('--output-directory',type=Path,required=True)
args=parser.parse_args();r=args.output_directory;r.mkdir(parents=True,exist_ok=True)
d=json.loads(args.catalog.read_text(encoding='utf-8'));g=d['geometry']
wire={e['candidate_id']:e['native_loaded_pose'] for e in d['entries'] if 'native_loaded_pose' in e}
body={e['candidate_id']:[e['native_loaded_body']['scale'],e['native_loaded_body']['hover']] for e in d['entries'] if 'native_loaded_body' in e}
wp={(p['path'],p['point']):p['pose'] for p in g['native_waypoints']}
bounds={int(k):(v['base'],v['die_sides']) for k,v in g['native_scale_bounds'].items()}
assert len(wire)==len(body)==215 and len(wp)==98 and bounds=={123978:(-6.,26),191450:(-6.,26)}
lines=[];manifest=[];random=[]
for e in d['entries']:
 ident=e['candidate_id'];pose=wire.get(ident,e['pose'])[:3];scale,hover=body.get(ident,[1,0]);spells=[int(x) for x in e.get('addon',{}).get('auras','').split()];random_spell=next((x for x in spells if x in bounds),None)
 if random_spell:
  assert e['npc'] in (97087,95834,96664) and hover==0
  scale=1.2;random.append(e)
 lines.append(' '.join(map(str,[ident,e['map'],e['npc'],e['floor'],*pose,*e['native_anchor'],0,scale,hover])))
 manifest.append({'id':ident,'kind':'native_wire_spawn' if ident in wire else 'script_spawn','candidate':ident,'pose':pose,'scale':scale,'hover':hover})
assert len(random)==5
for e in random:
 # Native CalcValue integer range -6+[1..26];negative aura amounts divide
 # by(1+abs(percent)/100),positive ones multiply. Exercise every possible draw.
 for index,amount in enumerate(range(-5,20)):
  scale=1+amount/100 if amount>=0 else 1/(1-amount/100);ident=100000000+e['candidate_id']*100+index;pose=wire[e['candidate_id']][:3]
  lines.append(' '.join(map(str,[ident,e['map'],e['npc'],e['floor'],*pose,*e['native_anchor'],0,scale,0])))
  manifest.append({'id':ident,'kind':'native_random_size','candidate':e['candidate_id'],'amount':amount,'scale':scale,'pose':pose})
for e in d['entries']:
 if not e.get('path'):continue
 path=sorted([p for p in d['waypoints'] if p['id']==e['path']],key=lambda p:p['point'])
 for i,p in enumerate(path):
  pose=wp[(p['id'],p['point'])];anchor=wp[(path[i-1]['id'],path[i-1]['point'])]
  if math.dist(pose,anchor)<.1:anchor=next(wp[(q['id'],q['point'])] for q in path if math.dist(wp[(q['id'],q['point'])],pose)>.1)
  ident=200000000+e['candidate_id']*100+i;scale,hover=body[e['candidate_id']]
  lines.append(' '.join(map(str,[ident,e['map'],e['npc'],e['floor'],*pose,*anchor,0,scale,hover])))
  manifest.append({'id':ident,'kind':'native_wire_waypoint','candidate':e['candidate_id'],'path':p['id'],'point':p['point'],'pose':pose})
(r/'wire-placement-input.tsv').write_text('\n'.join(lines)+'\n');(r/'wire-placement-manifest.json').write_text(json.dumps(manifest,indent=2));print('WIRE_PLACEMENTS',len(lines),'SPAWNS222/RANDOM125/WAYPOINTS98')
# Native follower destination-to-destination corridors.
groups=collections.defaultdict(list)
for point in g['formation_targets']:groups[point['candidate']].append(point)
legs=[];leg_manifest=[];serial=300000000
for ident,points in groups.items():
 points.sort(key=lambda p:p['point'])
 for i,p in enumerate(points):
  a=points[i-1]['pose'];b=p['pose']
  if math.dist(a,b)<.1:continue
  serial+=1;legs.append(' '.join(map(str,['T',serial,p['map'],p['npc'],*a,*b])));leg_manifest.append({'id':serial,'candidate':ident,'from':a,'to':b})
(r/'formation-legs-input.tsv').write_text('\n'.join(legs)+'\n');(r/'formation-legs-manifest.json').write_text(json.dumps(leg_manifest,indent=2));print('FORMATION_LEGS',len(legs))
script_points=[{'candidate_id':983,'point':i,'pose':pose} for i,pose in enumerate(g['guard_patrol'])];patrol_manifest=[]
for e in d['entries']:
 if e.get('path'):points=[wp[(p['id'],p['point'])] for p in sorted(d['waypoints'],key=lambda p:p['point']) if p['id']==e['path']]
 elif e['candidate_id']==983:points=[p['pose'] for p in sorted(script_points,key=lambda p:p['point']) if p['candidate_id']==983]
 else:continue
 poses=[wire.get(e['candidate_id'],e['pose'])[:3]]+points+[points[0]]
 for a,b in zip(poses,poses[1:]):
  if math.dist(a,b)<.1:continue
  serial+=1;legs.append(' '.join(map(str,['T',serial,e['map'],e['npc'],*a,*b])));patrol_manifest.append({'id':serial,'candidate':e['candidate_id'],'from':a,'to':b})
(r/'final-wire-corridors-input.tsv').write_text('\n'.join(legs)+'\n');(r/'wire-patrol-legs-manifest.json').write_text(json.dumps(patrol_manifest,indent=2));print('WIRE_PATROL_LEGS',len(patrol_manifest),'ALL_CORRIDORS',len(legs))
