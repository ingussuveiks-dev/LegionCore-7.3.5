"""Independent declared amendments for the requested ten groups of ten.

Only implemented entries are checked;the authorized target is not a completion
claim. Historical certificates retain the exact pre-task source view.
"""
from pathlib import Path
import functools,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/TenTen20261111Repairs.json').read_text(encoding='utf8'))
def normalize(path,source):
 for row in reversed(PLAN['changes']):
  for edit in reversed(row['edits']):
   if edit['path']!=path:continue
   count=edit.get('count',1)
   if source.count(edit['new'])==count:source=source.replace(edit['new'],edit['old'],count)
   else:assert source.count(edit['old'])==count,(row['key'],'undeclared/partial amendment')
 return source
@functools.lru_cache(None)
def verify():
 assert PLAN['authorized_total']==100 and len(PLAN['changes'])<=100 and len({e['key']for e in PLAN['changes']})==len(PLAN['changes'])
 for path in sorted({p for row in PLAN['changes']for p in row['paths']}):
  source=(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape');old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig',errors='surrogateescape').replace('\r\n','\n');assert normalize(path,source)==old,'Undeclared amendment:'+path
 return True
def retained_source(path,source=None):
 verify();return normalize(path,source if source is not None else(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape'))
def require_fixed(keys=None):
 verify();selected=[e for e in PLAN['changes']if keys is None or e['key']in keys]
 sources={p:(ROOT/p).read_text(encoding='utf-8-sig',errors='surrogateescape')for e in PLAN['changes']for p in e['paths']}
 chosen={e['key']for e in selected}
 for e in reversed(PLAN['changes']):
  for edit in reversed(e['edits']):
   path=edit['path'];count=edit.get('count',1)
   if e['key']in chosen:assert sources[path].count(edit['new'])==count,e['key']
   sources[path]=sources[path].replace(edit['new'],edit['old'],count)
 print('PASS exact source scope for',len(selected),'implemented selected repairs;authorized total100 is separate')
if __name__=='__main__':require_fixed()
