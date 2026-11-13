"""Exact declared changes for the second requested hundred; completion is separate."""
from pathlib import Path
import functools,json,subprocess
from pet_lifecycle_scope import normalize as normalize_lifecycle,verify as verify_lifecycle
from warswords_scene_expiry_scope import normalize as normalize_scene_expiry,verify as verify_scene_expiry
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/SecondTenTen20261112Repairs.json').read_text(encoding='utf8'))
def normalize(path,source):
 source=normalize_lifecycle(path,normalize_scene_expiry(path,source))
 for row in reversed(PLAN['changes']):
  for edit in reversed(row['edits']):
   if edit['path']!=path:continue
   count=edit.get('count',1)
   if source.count(edit['new'])==count:source=source.replace(edit['new'],edit['old'],count)
   else:assert source.count(edit['old'])==count,(row['key'],'undeclared/partial amendment')
 return source
@functools.lru_cache(None)
def verify():
 verify_scene_expiry()
 verify_lifecycle()
 assert PLAN['authorized_total']==100 and len(PLAN['changes'])<=100
 assert len({e['key']for e in PLAN['changes']})==len(PLAN['changes'])
 for path in sorted({p for row in PLAN['changes']for p in row['paths']}):
  s=(ROOT/path).read_text(encoding='utf-8-sig',errors='surrogateescape')
  old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig',errors='surrogateescape').replace('\r\n','\n')
  assert normalize(path,s)==old,'Undeclared amendment:'+path
 return True
def require_fixed(keys=None):
 verify()
 print('PASS exact declared second-hundred source scope; implemented candidates',len(PLAN['changes']),'are not a completion claim')
if __name__=='__main__':require_fixed()
