"""Exact native team accuracy targeting amendment and pinned whole-file view."""
from pathlib import Path
import functools,hashlib,json,subprocess
from pet_ward_rot_scope import normalize as normalize_types,verify as verify_types
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/PetBattleAccuracyRosterAmendments.json').read_text(encoding='utf8'))
def restore(path,source):
 for edit in reversed(PLAN['edits']):
  if edit['path']!=path:continue
  if source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
  else:assert source.count(edit['old'])==1,('Partial native accuracy roster amendment',path)
 return source
def normalize(path,source):return restore(path,normalize_types(path,source))
@functools.lru_cache(None)
def verify():
 verify_types()
 for path,hashes in PLAN['files'].items():
  source=normalize_types(path,(ROOT/path).read_text(encoding='utf-8-sig'))
  pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(source.encode()).hexdigest()==hashes['after_sha256'],('Undeclared accuracy roster amendment',path)
  assert restore(path,source)==pinned,('Accuracy roster whole-file restoration differs',path)
 return True
if __name__=='__main__':verify();print('PASS exact native accuracy roster targeting amendment and independently pinned prior whole file')
