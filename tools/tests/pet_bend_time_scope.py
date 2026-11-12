"""Independently declared native Bend Time amendment to earlier scopes."""
from pathlib import Path
import functools,hashlib,json,subprocess
from pet_round_bits_scope import normalize as normalize_bits,verify as verify_bits
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/PetBattleBendTimeAmendments.json').read_text(encoding='utf8'))
def restore(path,source):
 for edit in reversed(PLAN['edits']):
  if edit['path']!=path:continue
  if source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
  else:assert source.count(edit['old'])==1,('Partial native Bend Time amendment',path)
 return source
def normalize(path,source):return restore(path,normalize_bits(path,source))
@functools.lru_cache(None)
def verify():
 verify_bits()
 for path,hashes in PLAN['files'].items():
  source=normalize_bits(path,(ROOT/path).read_text(encoding='utf-8-sig'))
  pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(source.encode()).hexdigest()==hashes['after_sha256'],('Undeclared Bend Time amendment',path)
  assert restore(path,source)==pinned,('Bend Time whole-file restoration differs',path)
 return True
if __name__=='__main__':verify();print('PASS exact declared native Bend Time amendment; earlier source fixtures retain their independently pinned view')
