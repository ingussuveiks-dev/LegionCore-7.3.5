"""Exact current middle-dialogue delta; retained progression hosts stay pinned."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/TuureRouteMiddleRepair.json').read_text())
def inverse(source):
 for edit in reversed(PLAN['edits']):
  assert source.count(edit['new'])==1,'Partial or foreign Tuure middle-dialogue amendment'
  source=source.replace(edit['new'],edit['old'],1)
 return source
@functools.lru_cache(None)
def verify():
 source=(ROOT/PLAN['path']).read_text(encoding='utf-8-sig')
 header=(ROOT/PLAN['header']).read_text()
 if '#include "tuure_route_opening.h"' in source:
  from tuure_route_opening_scope import verify as opening
  restored=opening();source=restored[PLAN['path']];header=restored[PLAN['header']]
 old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+PLAN['path']]).decode('utf-8-sig').replace('\r\n','\n')
 sha=lambda s:hashlib.sha256(s.encode()).hexdigest()
 assert sha(source)==PLAN['after_sha256'] and sha(old)==PLAN['before_sha256']
 assert sha(header)==PLAN['header_sha256']
 assert inverse(source)==old,'Other Tuure progression,death,captain,cage or recovery changed'
 return True
if __name__=='__main__':
 verify();print('PASS exact Tuure middle-dialogue delta;retained first-fight/route/cage/captain/recovery source unchanged')
