"""Pinned complete bound logout and detached journal lifecycle."""
from pathlib import Path
import functools,hashlib,json,subprocess
from five_ten_20261113_scope import normalize as normalize_later,verify as verify_later
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/PetBattleLogoutAmendments.json').read_text(encoding='utf8'))
def restore(path,source):
 for edit in reversed(PLAN['edits']):
  if edit['path']!=path:continue
  if source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
  else:assert source.count(edit['old'])==1,('Partial native Logout amendment',path)
 return source
def normalize(path,source):return restore(path,normalize_later(path,source))
@functools.lru_cache(None)
def verify():
 verify_later()
 for path,hashes in PLAN['files'].items():
  source=normalize_later(path,(ROOT/path).read_text(encoding='utf-8-sig'))
  pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(source.encode()).hexdigest()==hashes['after_sha256'],('Undeclared Logout amendment',path)
  assert restore(path,source)==pinned,('Logout whole-file restoration differs',path)
 return True
if __name__=='__main__':verify();print('PASS exact native Logout amendment and independently pinned prior whole files')
