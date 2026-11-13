"""Exact authorized reconstruction delta; historical suites retain their scope."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/SharasdalLivingFinaleRepair.json').read_text(encoding='utf-8'))
def normalize(path,source):
 for edit in PLAN['edits']:
  if edit['path']==path and source.count(edit['new'])==1:
   source=source.replace(edit['new'],edit['old'],1)
 return source
@functools.lru_cache(None)
def verify():
 for path,hashes in PLAN['files'].items():
  current=(ROOT/path).read_text(encoding='utf-8-sig')
  old='' if hashes['added'] else subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(old.encode()).hexdigest()==hashes['before_sha256'],path
  assert hashlib.sha256(current.encode()).hexdigest()==hashes['after_sha256'],('Undeclared finale change',path)
  if not hashes['added']:assert normalize(path,current)==old,('Other actor/route/progression changed',path)
 return True
if __name__=='__main__':
 verify();print('PASS exact authorized living-Lady reconstruction source delta;client and deployed binary checks separate')
