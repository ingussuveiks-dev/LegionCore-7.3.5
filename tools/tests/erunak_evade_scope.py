"""Certify and invert only the Erunak evade delta for historical source suites."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/SharasdalErunakEvadeRepair.json').read_text())
def inverse(source):
 for edit in PLAN['edits']:
  assert source.count(edit['new'])==1,'Missing or duplicate declared Erunak delta'
  source=source.replace(edit['new'],edit['old'],1)
 return source
@functools.lru_cache(None)
def verify():
 current=(ROOT/PLAN['path']).read_text(encoding='utf-8-sig')
 old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+PLAN['path']]).decode('utf-8-sig').replace('\r\n','\n')
 sha=lambda s:hashlib.sha256(s.encode()).hexdigest()
 assert sha(old)==PLAN['before_sha256'] and sha(current)==PLAN['after_sha256']
 assert sha((ROOT/PLAN['added_header']).read_text())==PLAN['header_sha256']
 assert inverse(current)==old,'Other companion,actor,combat or progression changed'
 return True
def normalize(path,source):
 if path==PLAN['path'] and '#include "sharasdal_erunak_evade.h"' in source:
  verify();return inverse(source)
 return source
if __name__=='__main__':
 verify();print('PASS exact Erunak combat-to-route delta;historical actor and progression source unchanged')
