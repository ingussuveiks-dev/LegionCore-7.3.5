"""Certify the complete storage amendment before retaining historical hosts."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/AshbringerStorageRepair.json').read_text(encoding='utf-8'))
def normalize(path,source):
 for edit in PLAN['edits']:
  if edit['path']==path and source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
 return source
@functools.lru_cache(None)
def verify():
 for path,record in PLAN['files'].items():
  current=(ROOT/path).read_text(encoding='utf-8-sig')
  old='' if record['added'] else subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(old.encode()).hexdigest()==record['before_sha256'],path
  assert hashlib.sha256(current.encode()).hexdigest()==record['after_sha256'],('Undeclared storage amendment',path)
  if not record['added']:assert normalize(path,current)==old,('Other artifact/encounter behavior changed',path)
 return True
if __name__=='__main__':
 verify();print('PASS exact current Ashbringer storage amendment;historical host views remain independently pinned')
