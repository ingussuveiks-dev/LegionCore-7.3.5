"""Only declared repairs count towards the requested five groups of ten."""
from pathlib import Path
import functools,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/FiveTen20261113Repairs.json').read_text(encoding='utf8'))
def normalize(path,source):
 for row in reversed(PLAN['changes']):
  if row['path']!=path:continue
  if source.count(row['new'])==1:source=source.replace(row['new'],row['old'],1)
  else:assert source.count(row['old'])==1,('Undeclared/partial repair',row['key'])
 for row in reversed(PLAN.get('support',[])):
  if row['path']!=path:continue
  if source.count(row['new'])==1:source=source.replace(row['new'],row['old'],1)
  else:assert row['old'] and source.count(row['old'])==1,('Undeclared support change',path)
 return source
@functools.lru_cache(None)
def verify():
 assert PLAN['authorized_total']==50 and len(PLAN['changes'])<=50
 assert len({x['key']for x in PLAN['changes']})==len(PLAN['changes'])
 for path in sorted({x['path']for x in PLAN['changes']+PLAN.get('support',[])}):
  s=(ROOT/path).read_text(encoding='utf-8-sig')
  pinned=subprocess.run(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path],capture_output=True)
  if pinned.returncode:
   assert any(x['path']==path and x['old']==''for x in PLAN.get('support',[])) and not subprocess.check_output(['git','-C',str(ROOT),'ls-tree','--name-only',PLAN['baseline'],'--',path]).strip()
   old=''
  else:old=pinned.stdout.decode('utf-8-sig').replace('\r\n','\n')
  assert normalize(path,s)==old,('Undeclared file delta',path)
 return True
if __name__=='__main__':verify();print('PASS exact declared source scope;implemented',len(PLAN['changes']),'authorized50;completion separate')
