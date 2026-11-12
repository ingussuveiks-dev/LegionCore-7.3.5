"""Complete changed callbacks; external encounter endpoints are controlled explicitly."""
from pathlib import Path
import sys,re,json
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True)
rows=[r for r in PLAN['changes']if r['batch']==5];assert len(rows)==10
parts=[];provenance=[]
for i,row in enumerate(rows):
 source=row['edits'][0]['old'if mode=='baseline'else'new'];fields=row['fixture_fields']
 if mode=='baseline':
  fields=fields.replace('ObjectGuid UghostGUID;','Creature* Ughost=nullptr; Creature* Theldanis=nullptr;').replace('ObjectGuid timeRiftGuid;','Creature* timeRift=nullptr;').replace('ObjectGuid ownerGuid;','Player* ow=nullptr;'if i==2 else'ObjectGuid ownerGuid;').replace('ObjectGuid ownerGuid,targetGuid;','Unit* owner=nullptr; Unit* target=nullptr;').replace('ObjectGuid riderGuid;','Player* plrRide=nullptr;').replace('ObjectGuid illidanGuid;','Creature* illidan_saved=nullptr;').replace('ObjectGuid webTargetGuid;','Unit* plrWebTarget=nullptr;').replace('ObjectGuid elisandaGuid;','Creature* elisanda=nullptr;').replace('ObjectGuid kohcromGuid;','Creature* pKohcrom=nullptr;')
 if i==9 and mode=='baseline':fields+=' Creature* _target=nullptr;'
 if i==0:fields+=' Player* escort=nullptr;Player* GetPlayerForEscort(){return escort;}'
 if i==7:fields+=' bool introDone=false;'
 if i==8:fields+=' ObjectGuid _stompguid1,_stompguid2; bool bEnrage=false;'
 prefix=f'struct Fifth{i}: '+('GameObjectAI'if i==2 else'BossAI')+'{using '+('GameObjectAI'if i==2 else'BossAI')+'::'+('GameObjectAI'if i==2 else'BossAI')+';'+fields
 bodies=[]
 for method in row['methods']:
  body=block(source,r'void '+method+r'\(');bodies.append(body.replace(' override',''));provenance.append(dict(key=row['key'],method=method,path=row['paths'][0],scope='complete production method; override keyword omitted for fixture base'))
 parts.append(prefix+'\n'+'\n'.join(bodies)+'\n};')
text=('#define FIXED_ACTOR_CALLBACKS\n'if mode=='fixed'else'')+'\n'.join(parts);labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',text))|{'TYPEID_UNIT','TYPEID_PLAYER','IN_PROGRESS'}
fixed={'TYPEID_PLAYER':1,'TYPEID_UNIT':2,'POINT_MOTION_TYPE':1,'IN_MILLISECONDS':1000,'GO_STATE_READY':0,'IN_PROGRESS':1,'UNIT_STATE_STUNNED':1,'UNIT_STATE_ONVEHICLE':2,'NULL':0}
for name in sorted(labels-set(fixed)-{'RAND','AI','NPC','GO','M_PI'}):fixed[name]=len(fixed)+100
del fixed['NULL']
(out/'Labels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n')
(out/'Selected.inc').write_bytes(text.encode('utf8',errors='surrogateescape'));(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));(out/'provenance.json').write_text(json.dumps(provenance,indent=2))
print('Prepared complete affected callbacks for ten fifth-group repairs; motion/casts/quest credits/instance state are controlled, not native gameplay traversal.')
