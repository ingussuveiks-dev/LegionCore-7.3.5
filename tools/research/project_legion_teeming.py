"""Project pinned Legion MDT image points using typed native map metadata.

Reads public Lua and read-only World/native exports; never writes server data.
"""
import argparse,json,re,math,statistics,collections,hashlib
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--research-directory',type=Path,required=True)
parser.add_argument('--addon-directory',type=Path,required=True)
parser.add_argument('--source-catalog',type=Path,default=Path(__file__).resolve().parents[2]/'docs/research/legion-teeming-mdt-2018-05-27.json')
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=args.research_directory;routes=json.loads(args.source_catalog.read_text())['routes'];spawns=[json.loads(x) for x in (root/'creature.tsv').read_text(encoding='utf-8-sig').splitlines()];areas=[];floors=[]
for line in (root/'map-catalog/catalog.tsv').read_text(encoding='utf-8-sig').splitlines():
 a=line.split('\t')
 if a[0]=='AREA':areas.append({'id':int(a[1]),'map':int(a[2]),'left':float(a[4]),'right':float(a[5]),'top':float(a[6]),'bottom':float(a[7]),'name':a[9]})
 if a[0]=='FLOOR':floors.append({'id':int(a[1]),'map':int(a[2]),'floor':int(a[4]),'min_y':float(a[7]),'min_x':float(a[8]),'max_y':float(a[9]),'max_x':float(a[10])})
code=(root/'MethodDungeonTools.lua').read_text(encoding='utf-8-sig');block=code.split('MethodDungeonTools.dungeonMaps = {',1)[1].split('\n}',1)[0];maps={}
for match in re.finditer(r'\[(\d+)\]\s*=\s*\{(.*?)\n\s*\},',block,re.S):maps[int(match[1])]={int(i):v for i,v in re.findall(r'\[(\d+)\]\s*=\s*"([^"]+)"',match[2])}
width=1002*(840/4+4)/256;height=668*(840/4+4)/256
points=[];reports=[]
for route in routes:
 source=args.addon_directory/route['source_file'];raw=source.read_bytes();assert hashlib.sha256(raw).hexdigest()==route['source_sha256'],'Pinned addon source hash mismatch';text=raw.decode('utf-8-sig');index=int(re.search(r'local dungeonIndex\s*=\s*(\d+)',text)[1]);npc=None;name=None;start=len(points)
 for line in text.split('dungeonEnemies',1)[1].splitlines():
  n=re.search(r'\["name"\]\s*=\s*"([^"]+)"',line)
  if n:name=n[1]
  m=re.search(r'\["id"\]\s*=\s*(\d+)',line)
  if m:npc=int(m[1])
  if not re.match(r'\s*\[\d+\]\s*=\s*\{x\s*=',line):continue
  fields={}
  for field in re.finditer(r'([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^,}]+)',line):fields.setdefault(field[1],field[2].strip())
  if 'sublevel' not in fields:continue
  sub=int(fields['sublevel']);image=maps[index][sub]
  if image.lower()==maps[index][0].lower():
   area,=[a for a in areas if a['map']==route['map_id'] and a['name'].lower()==image.lower()];bounds={'max_x':area['top'],'min_x':area['bottom'],'max_y':area['left'],'min_y':area['right']};record_id='area:'+str(area['id'])
  else:
   floor=int(re.search(r'(\d+)_$',image)[1]);bounds,=[f for f in floors if f['map']==route['map_id'] and f['floor']==floor];record_id='floor:'+str(bounds['id'])
  x=bounds['max_x']+(-float(fields['y'])/height)*(bounds['min_x']-bounds['max_x']);y=bounds['max_y']+(float(fields['x'])/width)*(bounds['min_y']-bounds['max_y'])
  candidates=[s for s in spawns if s['map']==route['map_id'] and s['id']==npc and s['spawnMask']&256]
  best=min(candidates,key=lambda s:math.hypot(s['position_x']-x,s['position_y']-y)) if candidates else None
  points.append({'map':route['map_id'],'route':route['challenge_id'],'npc':npc,'name':name,'sublevel':sub,'group':int(fields['g']) if 'g' in fields else None,'clone':int(re.match(r'\s*\[(\d+)\]',line)[1]),'teeming':fields.get('teeming')=='true','addon_x':fields['x'],'addon_y':fields['y'],'bounds':record_id,'x':x,'y':y,'closest_guid':best['guid'] if best else None,'closest_distance':math.hypot(best['position_x']-x,best['position_y']-y) if best else None,'closest_z':best['position_z'] if best else None})
 sample=points[start:];normal=[p['closest_distance'] for p in sample if not p['teeming'] and p['closest_distance'] is not None];extra=[p['closest_distance'] for p in sample if p['teeming'] and p['closest_distance'] is not None];normal.sort();extra.sort()
 report={'route':route['challenge_id'],'map':route['map_id'],'normal_points':sum(not p['teeming'] for p in sample),'teeming_points':sum(p['teeming'] for p in sample),'normal_median_distance':statistics.median(normal),'normal_p90_distance':normal[int(len(normal)*.9)],'normal_within5':sum(d<=5 for d in normal),'normal_within10':sum(d<=10 for d in normal),'teeming_median_distance':statistics.median(extra),'teeming_within2':sum(d<=2 for d in extra),'teeming_within5':sum(d<=5 for d in extra)};reports.append(report);print(json.dumps(report),flush=True)
assert sum(p['teeming'] for p in points)==sum(r['extra_point_count'] for r in routes)==222
lookup={(r['challenge_id'],p['npc_id'],p['source_clone_index']):p for r in routes for p in r['points']}
for p in points:
 if p['teeming']:
  original=lookup[(p['route'],p['npc'],p['clone'])];assert (p['addon_x'],p['addon_y'])==(original['addon_x'],original['addon_y']),'Projection lost outer clone coordinates'
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps({'width':width,'height':height,'points':points,'reports':reports},indent=2)+'\n')
