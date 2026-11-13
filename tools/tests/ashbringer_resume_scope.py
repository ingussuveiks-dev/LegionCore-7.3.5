"""Exact original-AI selection in the existing domination removal callback."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/AshbringerResumeRepair.json').read_text(encoding='utf-8'))
@functools.lru_cache(None)
def verify():
 old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+PLAN['path']]).decode().replace('\r\n','\n')
 current=(ROOT/PLAN['path']).read_text(encoding='utf-8-sig')
 for value,key in ((old,'before_sha256'),(current,'after_sha256')):assert hashlib.sha256(value.encode()).hexdigest()==PLAN[key]
 assert current.count(PLAN['new'])==1 and current.replace(PLAN['new'],PLAN['old'],1)==old
 return True
