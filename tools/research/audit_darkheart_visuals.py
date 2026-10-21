"""Trace Darkheart warning effects through native 7.3.5 WDC1 tables and read-only CASC model exports."""
import sys,re,json,struct,argparse,hashlib
from pathlib import Path
sys.path.insert(0,'tools/research');sys.dont_write_bytecode=True
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--models',type=Path,required=True,help='Directory containing read-only FileXXXXXXXX.bin CASC exports')
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
d=Data(Path('build-extractors/bin/Release/dbc/enUS'),Path('__native_only__'))
def parented(name):
 meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.I)[1].split('\n};')[0]
 index,layout,parent=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+), types, arraySizes, (-?\d+)',meta).groups()
 index,parent=int(index),int(parent)
 types=re.search(r'types = "([^"]+)"',meta)[1]
 sizes=[int(v) for v in re.search(r'arraySizes\[\d+\] = \{([^}]+)',meta)[1].split(',')]
 info=re.split('struct '+name+'LoadInfo\n',d.load,flags=re.I)[1].split('\n};')[0]
 fields=re.findall(r'\{ (true|false), FT_\w+, "([^"]+)" \}',info)
 if index<0:fields=[f for f in fields if f[1]!='ID']
 if len(fields)+1==sum(sizes):
  assert parent>=0 and sizes[parent]==1
  fields.insert(sum(sizes[:parent]),('true','parent'))
 assert len(fields)==sum(sizes)
 table=Table(d.dbc/(name+'.db2'),int(index),int(layout,16))
 rows={}
 for i,id in enumerate(table.ids):
  row={'ID':id};off=0
  for col,(kind,count) in enumerate(zip(types,sizes)):
   raw=table.value(i,col) if col<table.fields else table.parents[i]
   width={'i':32,'f':32,'h':16,'b':8}[kind]
   values=raw if isinstance(raw,list) else [(raw>>(width*j))&((1<<width)-1) for j in range(count)]
   for (signed,field),value in zip(fields[off:off+count],values):
    if kind=='f':value=struct.unpack('<f',struct.pack('<I',value))[0]
    elif signed=='true' and value&(1<<(width-1)):value-=1<<width
    row[field]=value
   off+=count
  rows[id]=row
 for dst,src in table.copy_pairs:rows[dst]=dict(rows[src],ID=dst)
 return list(rows.values())
spellids={198937,201145,198915,198916,220369}
links=[r for r in d.rows('SpellXSpellVisual').values() if r['SpellID'] in spellids]
visualids={r['SpellVisualID'] for r in links}
events=[r for r in parented('SpellVisualEvent') if r['parent'] in visualids]
kits={r['SpellVisualKitID'] for r in events}
effects=[r for r in parented('SpellVisualKitEffect') if r['parent'] in kits]
attachids={r['Effect'] for r in effects if r['EffectType']==2}
attachments=[r for r in parented('SpellVisualKitModelAttach') if r['ID'] in attachids]
nameids={r['SpellVisualEffectNameID'] for r in attachments}
# EffectName uses the actual 7.3.5.26972 layout, B930A934. The core's
# LoadInfo for this otherwise-unused table has a later schema. Do not guess
# field offsets from that schema or change production DB2 structures here.
table=Table(d.dbc/'SpellVisualEffectName.db2',-1,0xB930A934)
models=[dict(ID=id,ModelFileDataID=table.value(i,10),TextureFileDataID=table.value(i,8),Scale=struct.unpack('<f',struct.pack('<I',table.value(i,2)))[0],Alpha=struct.unpack('<f',struct.pack('<I',table.value(i,5)))[0]) for i,id in enumerate(table.ids) if id in nameids]
for model in models:
 raw=(args.models/('File%08X.bin'%model['ModelFileDataID'])).read_bytes()
 m=raw[8:] if raw[:4]==b'MD21' else raw
 assert m[:4]==b'MD20'
 n,o=struct.unpack_from('<II',m,8)
 model['name']=m[o:o+n].rstrip(b'\0').decode()
 n,o=struct.unpack_from('<II',m,28)
 model['animations']=[dict(zip(('id','variation','duration_ms'),struct.unpack_from('<HHI',m,o+i*64))) for i in range(n)]
 model['sha256']=hashlib.sha256(raw).hexdigest()
source_hashes={name:hashlib.sha256((d.dbc/(name+'.db2')).read_bytes()).hexdigest() for name in ('SpellXSpellVisual','SpellVisualEvent','SpellVisualKitEffect','SpellVisualKitModelAttach','SpellVisualEffectName')}
result=dict(build='7.3.5.26972',links=links,events=events,effects=effects,attachments=attachments,models=models,
 missing_effect_names=sorted(nameids-{m['ID'] for m in models}),source_sha256=source_hashes,client_rendering_verified=False)
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps(result,indent=2)+'\n')
print('PASS: native visual/event/kit/attachment/model/animation trace saved; rendering remains unverified.')