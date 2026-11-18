"""Classify read-only native surface observations; no spawn approval or SQL.

The additional proximity screen is conservative research screening, not a
replacement for native path generation or client traversal.
"""
import argparse,json,collections,math
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--projection',type=Path,required=True)
parser.add_argument('--native-catalog',type=Path,required=True)
parser.add_argument('--surfaces',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
projection=json.loads(args.projection.read_text())['points'];chunks=[]
for line in args.native_catalog.read_text(encoding='utf-8-sig').splitlines():
 a=line.split('\t')
 if a[0]=='CHUNK':chunks.append({'map':int(a[2]),'floor':int(a[3]),'min_z':float(a[4]),'group':int(a[6])})
def floor_of(mapid,z,group):
 matches=[x for x in chunks if x['map']==mapid and x['group']==group and z>=x['min_z']]
 if not matches:return []
 high=max(x['min_z'] for x in matches);return sorted({x['floor'] for x in matches if x['min_z']==high})
surfaces=[]
source=args.surfaces
for line in source.read_text(encoding='utf-8-sig').splitlines():
 a=line.split('\t')
 if a[0]!='SURFACE':continue
 row={'kind':a[1],'id':int(a[2]),'map':int(a[3]),'entry':int(a[4]),'x':float(a[5]),'y':float(a[6]),'z':float(a[7]),'area':bool(int(a[8])),'adt':int(a[9]),'root':int(a[10]),'group':int(a[11]),'nav_query':bool(int(a[13])),'nav':bool(int(a[14])),'nx':float(a[15]),'ny':float(a[16]),'nz':float(a[17]),'nav_flags':int(a[18]),'height':float(a[19]),'radius':float(a[20]),'body':bool(int(a[21])),'center':bool(int(a[22])),'foot':bool(int(a[23])),'foot_delta':float(a[24])};row.update({'support_entry':int(a[25]) if len(a)>25 else 0,'model_query':bool(int(a[26])) if len(a)>26 else False,'model_nav':bool(int(a[27])) if len(a)>27 else False,'model_nearest':[float(x) for x in a[28:31]] if len(a)>31 else None,'model_flags':int(a[31]) if len(a)>31 else 0});row['floors']=floor_of(row['map'],row['z'],row['group']);surfaces.append(row)
normal={s['id']:s for s in surfaces if s['kind']=='N'};out=[];stats=collections.Counter()
for i,p in enumerate(projection):
 if not p['teeming']:continue
 floor=int(p['bounds'].split(':')[1]) if p['bounds'].startswith('floor:') else 0
 observations=[s for s in surfaces if s['kind']=='C' and s['id']==i]
 candidates=[s for s in observations if (floor in s['floors'] if floor else not s['floors']) and (s['nav'] or s['model_nav'])]
 selected=min(candidates,key=lambda s:(not(s['center'] and s['foot']),abs(s['z']-p['closest_z']))) if candidates else None
 status='native_floor_nav' if selected else 'unresolved_floor_nav'
 if selected and selected['center'] and selected['foot']:status='native_floor_nav_body'
 stats[status]+=1
 out.append({'id':i,'point':p,'status':status,'expected_floor':floor,'selected':selected,'surfaces':observations})
for route in sorted({r['point']['route'] for r in out}):
 print(route,dict(collections.Counter(x['status'] for x in out if x['point']['route']==route)))
print('TOTAL',dict(stats));print('UNRESOLVED',[(x['id'],x['point']['npc'],x['expected_floor'],len(x['surfaces'])) for x in out if x['status']=='unresolved_floor_nav'])
assert len(out)==222,'Incomplete pinned point coverage'
strict=0
for row in out:
 s=row['selected']
 if not s:continue
 near=s['model_nearest'] if s['model_nav'] else [s['nx'],s['ny'],s['nz']]
 row['nearest_nav_xy_distance']=math.hypot(near[0]-s['x'],near[1]-s['y']);row['nearest_nav_z_delta']=abs(near[2]-s['z'])
 row['conservative_proximity_screen']=row['status']=='native_floor_nav_body' and row['nearest_nav_xy_distance']<=3 and row['nearest_nav_z_delta']<=0.8
 strict+=row['conservative_proximity_screen']
print('CONSERVATIVE_PROXIMITY_SCREEN',strict,'remaining',len(out)-strict)
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps({'normal_surfaces':list(normal.values()),'candidates':out,'stats':dict(stats),'conservative_proximity_screen':strict,'server_spawns_approved':0},indent=2)+'\n')
