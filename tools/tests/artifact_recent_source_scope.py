"""Exact Git-proven inverses for retained artifact test source views.

These certify source provenance; they do not execute the active Warrior flow.
"""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
NAMES=('ArtifactRecentPlayerSourceRepairs.json','ArtifactRecentWarswordsSourceRepairs.json','ArtifactRecentFuryInstanceSourceRepairs.json','ArtifactRecentSharedHelpersSourceRepairs.json','ArtifactInventoryPlayerHeaderSourceRepair.json')
PLANS=[json.loads((ROOT/'tools/tests'/name).read_text()) for name in NAMES]
PLANS.extend(json.loads((ROOT/'tools/tests/ArtifactInventorySourceRepairs.json').read_text()))
def sha(s):return hashlib.sha256(s.encode()).hexdigest()
def inverse(source,plan):
 for edit in reversed(plan['edits']):
  assert source.count(edit['new'])==1,'Partial recent artifact source amendment'
  source=source.replace(edit['new'],edit['old'],1)
 return source
@functools.lru_cache(None)
def verify():
 for record in PLANS:
  source=(ROOT/record['path']).read_text(encoding='utf-8-sig')
  for plan in reversed(record['plans']):
   assert sha(source)==plan['after_sha256'],('Undeclared recent source change',record['path'],plan['commit'])
   old=subprocess.check_output(['git','-C',str(ROOT),'show',plan['parent']+':'+record['path']]).decode('utf-8-sig').replace('\r\n','\n')
   assert sha(old)==plan['before_sha256']
   source=inverse(source,plan);assert source==old
 return True
def normalize(path,source):
 for record in PLANS:
  if path!=record['path']:continue
  verify()
  if sha(source)==record['plans'][-1]['after_sha256']:
   for plan in reversed(record['plans']):source=inverse(source,plan)
 return source
if __name__=='__main__':
 verify();print('PASS exact declared artifact source amendments against pinned Git parents; historical whole-file checks retained; no gameplay certification')
