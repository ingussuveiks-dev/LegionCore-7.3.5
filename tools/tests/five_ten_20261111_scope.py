"""Exact later amendments; keep historical source certificates independent.

Only the declared deltas normalize to the pinned pre-task tree. Behavioral
fixtures read the repaired sources directly and never use this retained view.
"""
from pathlib import Path
import json,subprocess,functools
from ten_ten_20261111_scope import retained_source as before_ten_ten
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/FiveTen20261111Repairs.json').read_text(encoding='utf8'))

def normalize(path,source):
    for row in reversed(PLAN['changes']):
        for edit in reversed(row['edits']):
            if edit['path']!=path:continue
            count=edit.get('count',1)
            if source.count(edit['new'])==count:
                source=source.replace(edit['new'],edit['old'],count)
            else:
                assert source.count(edit['old'])==count,(row['key'],'undeclared/partial delta')
    return source

@functools.lru_cache(None)
def verify():
    paths=sorted({p for row in PLAN['changes'] for p in row['paths']})
    assert len(PLAN['changes'])==50 and len({row['key']for row in PLAN['changes']})==50
    for path in paths:
        actual=before_ten_ten(path)
        old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
        assert normalize(path,actual)==old,'Undeclared amendment: '+path
    return True

def retained_source(path,source=None):
    verify()
    return normalize(path,before_ten_ten(path,source))

def require_fixed(keys=None):
    verify()
    for row in PLAN['changes']:
        if keys is not None and row['key']not in keys:continue
        for edit in row['edits']:
            assert (ROOT/edit['path']).read_text(encoding='utf-8-sig').count(edit['new'])==edit.get('count',1),row['key']
    print('PASS '+('all 50'if keys is None else str(len(keys))+' selected')+' declared repaired deltas; complete affected files normalize exactly to pinned '+PLAN['baseline'])

if __name__=='__main__':require_fixed()
