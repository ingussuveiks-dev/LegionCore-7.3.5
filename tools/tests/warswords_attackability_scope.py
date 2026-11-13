"""Exact final-Vigfus intro attackability amendment."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/WarswordsAttackabilityRepair.json').read_text(encoding='utf-8'))
def normalize(path,source):
 if path==PLAN['path']:
  if source.count(PLAN['new'])==1:source=source.replace(PLAN['new'],PLAN['old'],1)
  else:assert source.count(PLAN['old'])==1 or 'me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);' not in source,'Partial final attackability amendment'
 return source
@functools.lru_cache(None)
def verify():
 pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+PLAN['path']]).decode('utf-8-sig').replace('\r\n','\n')
 current=(ROOT/PLAN['path']).read_text(encoding='utf-8-sig')
 assert hashlib.sha256(pinned.encode()).hexdigest()==PLAN['before_sha256']
 assert hashlib.sha256(current.encode()).hexdigest()==PLAN['after_sha256']
 assert normalize(PLAN['path'],current)==pinned
 return True
