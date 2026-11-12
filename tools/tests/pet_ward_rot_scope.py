"""Pinned complete native Ward/Rot and categorical type-state amendment."""
from pathlib import Path
import functools,hashlib,json,subprocess
from pet_discharge_scope import normalize as normalize_discharge,verify as verify_discharge
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/PetBattleWardRotAmendments.json').read_text(encoding='utf8'))
def restore(path,source):
 for edit in reversed(PLAN['edits']):
  if edit['path']!=path:continue
  if source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
  else:assert source.count(edit['old'])==1,('Partial native Ward/Rot amendment',path)
 return source
def normalize(path,source):return restore(path,normalize_discharge(path,source))
@functools.lru_cache(None)
def verify():
 verify_discharge()
 for path,hashes in PLAN['files'].items():
  source=normalize_discharge(path,(ROOT/path).read_text(encoding='utf-8-sig'))
  pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(source.encode()).hexdigest()==hashes['after_sha256'],('Undeclared Ward/Rot amendment',path)
  assert restore(path,source)==pinned,('Ward/Rot whole-file restoration differs',path)
 return True
if __name__=='__main__':verify();print('PASS exact native Ward/Rot amendment and independently pinned prior whole files')
