from pathlib import Path
import json,argparse,hashlib
from open_go_text_repair import load,controls,expr,ROOT

def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('prepare','verify','native'));ap.add_argument('--output',required=True);ap.add_argument('--sql-file',required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=True);plan=load();rows=plan['rows'];fp=expr(rows[0]);mig=Path(a.sql_file).read_text(encoding='utf-8')
 if a.mode=='native':
  decl=['struct NameRow{uint32 entry;char const* before;char const* after;};static NameRow const NameRows[]={{0,"",""}};static char const* NameIds="";static char const* NameMigrations[]={"DO 0"};','struct OpenRow{uint32 entry,mask;char const* oldCap;char const* newCap;char const* oldUnk;char const* newUnk;};static OpenRow const OpenRows[]={']
  for x in rows:
   vals=[]
   for f in ('castBarCaption','unk1'):vals.extend([x['values'][f].encode().hex().upper(),x['changes'].get(f,{}).get('after_hex',x['values'][f].encode().hex().upper())])
   decl.append('{'+str(x['entry'])+','+str((1 if 'castBarCaption' in x['changes'] else 0)+(2 if 'unk1' in x['changes'] else 0))+','+','.join('"'+v+'"' for v in vals)+'},')
  decl+=['};','static NameRow const Controls[]={'+','.join('{'+str(x['entry'])+',"","'+x['hex']+'"}' for x in controls())+'};']
  import re
  statements=[s for s in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",'\n'.join(l for l in mig.splitlines() if not l.startswith('--'))) if s.strip()]
  expressions=[]
  for s in statements:
   chunks=[s[i:i+16000] for i in range(0,len(s),16000)];expressions.append('std::string(R"OPENSQL('+chunks[0]+')OPENSQL")'+''.join('+R"OPENSQL('+c+')OPENSQL"' for c in chunks[1:]))
  decl.append('static std::string const OpenMigrations[]={'+','.join(expressions)+'};');(out/'GameObjectNameRows.inc').write_text('\n'.join(decl),encoding='utf-8');return
 def snapshot(tag,normalize):
  h='h'
  if normalize:h='CASE h '+' '.join(f"WHEN '{x['after_fingerprint']}' THEN '{x['before_fingerprint']}'" for x in rows)+' ELSE h END'
  return f"SELECT '{tag}',COUNT(*),SHA2(GROUP_CONCAT(z ORDER BY z SEPARATOR ','),256) FROM (SELECT {h} AS z FROM (SELECT {fp} AS h FROM gameobject_template g) open_raw) open_sig;"
 ids=','.join(map(str,sorted({x['entry'] for x in rows}|{x['entry'] for x in controls()}|{239692})));total=len(ids.split(','))
 setup=f"SET SESSION group_concat_max_len=10000000;CREATE TEMPORARY TABLE open_seed LIKE gameobject_template;INSERT INTO open_seed SELECT * FROM gameobject_template WHERE entry IN({ids});CREATE TEMPORARY TABLE gameobject_template LIKE open_seed;INSERT INTO gameobject_template SELECT * FROM open_seed;SET SESSION TRANSACTION READ ONLY;"+mig
 for x in rows:setup+=f"UPDATE gameobject_template g SET "+','.join(f"`{f}`=CONVERT(0x{c['before_hex']} USING utf8mb4)" for f,c in x['changes'].items())+f" WHERE @open_go_safe AND g.entry={x['entry']} AND {fp}='{x['after_fingerprint']}';"
 fixed=' OR '.join(f"(g.entry={x['entry']} AND {fp}='{x['after_fingerprint']}')" for x in rows)
 if a.mode=='prepare':
  cases=[dict(label='positive',refusal=False,sql=setup+snapshot('before',True)+mig+f"SELECT 'fixed',COUNT(*) FROM gameobject_template g WHERE {fixed};"+snapshot('after',True)+mig+snapshot('replay',True)+"SELECT 'EOF';")]
  row=next(x for x in rows if len(x['changes'])==2);entry=rows[0]['entry'];f=next(iter(row['changes']));variants={'metadata':f'UPDATE gameobject_template SET flags=flags+1 WHERE entry={entry};','foreign-text':f"UPDATE gameobject_template SET unk1='foreign' WHERE entry={entry};",'missing':f'DELETE FROM gameobject_template WHERE entry={entry};','partial-row':f"UPDATE gameobject_template SET `{f}`=CONVERT(0x{row['changes'][f]['after_hex']} USING utf8mb4) WHERE entry={row['entry']};"}
  for label,change in variants.items():cases.append(dict(label=label,refusal=True,sql=setup+change+snapshot('custom',False)+mig+snapshot('after',False)+"SELECT 'EOF';"))
  for c in cases:(out/(c['label']+'.sql')).write_text(c.pop('sql'),encoding='utf-8')
  (out/'cases.json').write_text(json.dumps(cases),encoding='utf-8');fields=','.join(f"'{c}',g.`{c}`" for c in rows[0]['values']);live=f"SET SESSION TRANSACTION READ ONLY;SELECT g.entry,JSON_OBJECT({fields}) FROM gameobject_template g WHERE entry IN({ids});SELECT 'updater',hash,state FROM updates WHERE name='2026_11_15_82_open_go_text.sql';SELECT 'EOF';";(out/'live.sql').write_text(live,encoding='utf-8');return
 cases=json.loads((out/'cases.json').read_text())
 for c in cases:
  lines=(out/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF';r=[s.split('\t') for s in lines[:-1]]
  if c['refusal']:assert len(r)==2 and r[0][1:]==r[1][1:],c['label']
  else:assert len(r)==4 and r[1]==['fixed','35'] and r[0][1:]==r[2][1:]==r[3][1:] and int(r[0][1])==total
 if a.live:
  lines=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF' and len(lines)==total+2;expected={x['entry']:dict(x['values']) for x in rows};prior={x['entry']:x for x in controls()}
  for x in rows:
   for f,c in x['changes'].items():expected[x['entry']][f]=c['after']
  for line in lines[:-2]:
   entry,s=line.split('\t',1);v=json.loads(s);entry=int(entry)
   if entry in expected:assert v==expected[entry],entry
   if entry in prior:assert v['name']==prior[entry]['name'],entry
  h=hashlib.sha1(mig.encode('utf-8')).hexdigest().upper();assert lines[-2]=='updater\t'+h+'\tRELEASED'
 (out/'result.json').write_text(json.dumps(dict(fields=37,rows=35,controls=553,refusals=4,live=a.live,persistent_writes=False)));print(f'PASS37/35 rows full-schema install/replay, {total} full-row controls,4 drift/partial-state refusals; live={a.live}')
if __name__=='__main__':main()
