"""Run every inherited ABI consumer and the complete prior/current PE chains.

These are read-only assertion modules against one immutable Release. Memoize
successful default runpy prefixes so each shared assertion module runs once;
never rewrite instructions or skip a distinct assertion module.
"""
from pathlib import Path
import runpy,traceback,os,json

directory=Path(__file__).resolve().parent
original=runpy.run_path
results={}
active=set()
def checked(path,init_globals=None,run_name=None):
 if init_globals is not None or run_name is not None:return original(path,init_globals=init_globals,run_name=run_name)
 key=str(Path(path).resolve()).lower()
 if key in results:return results[key]
 assert key not in active,'Verifier dependency cycle'
 active.add(key)
 try:
  result=original(path)
  results[key]=result
  return result
 finally:active.remove(key)

runpy.run_path=checked
failures=[]
try:
 roots=[directory/'verify_time_loop_release.py',directory/'verify_ten_ten_20261111_release.py']
 roots+=sorted(path for path in directory.glob('verify_*.py')if path.resolve()!=Path(__file__).resolve() and 'from linked_player_abi import'in path.read_text(encoding='utf8'))
 manifest=os.environ.get('LEGION_ABI_ROOTS_MANIFEST')
 if manifest:
  names=json.loads(Path(manifest).read_text());assert all(Path(name).name==name and name.startswith('verify_')and name.endswith('.py')for name in names);roots=[directory/name for name in names]
 for path in dict.fromkeys(roots):
  try:checked(str(path));print('PASS inherited ABI root',path.name)
  except Exception as error:
   where=traceback.extract_tb(error.__traceback__)[-1]
   failures.append((path.name,Path(where.filename).name,where.lineno,str(error)))
   print('UNRESOLVED inherited ABI root',failures[-1])
finally:runpy.run_path=original
output=os.environ.get('LEGION_ABI_RESULTS')
if output:Path(output).write_text(json.dumps(dict(failures=failures,executed_modules=len(results),roots=[path.name for path in dict.fromkeys(roots)]),indent=2)+'\n')
assert not failures,failures
print('PASS all inherited ABI roots and complete prior/current linked chains;',len(results),'distinct assertion modules executed;shared successful prefixes reused only for this immutable Release')
