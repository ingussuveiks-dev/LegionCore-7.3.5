"""Pinned complete Fury bonfire interaction amendment, not a client certificate."""
from pathlib import Path
import functools,hashlib,json,subprocess
from warswords_runestone_scope import normalize as normalize_rune,verify as verify_rune
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/WarswordsBonfireInteractionRepair.json').read_text(encoding='utf8'))
def normalize(path,source):
    source=normalize_rune(path,source)
    if path==PLAN['path']:
        if source.count(PLAN['new'])==1:source=source.replace(PLAN['new'],PLAN['old'],1)
        else:assert source.count(PLAN['old'])==1,('Partial bonfire interaction amendment',path)
    for edit in reversed(PLAN.get('support',[])):
        if edit['path']!=path:continue
        if source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
        else:assert source.count(edit['old'])==1,('Partial bonfire observation',path)
    return source
@functools.lru_cache(None)
def verify():
    verify_rune()
    files=PLAN.get('files',{PLAN['path']:dict(before_sha256=PLAN['before_sha256'],after_sha256=PLAN['after_sha256'])})
    for path,hashes in files.items():
        pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
        current=normalize_rune(path,(ROOT/path).read_text(encoding='utf-8-sig'))
        assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
        assert hashlib.sha256(current.encode()).hexdigest()==hashes['after_sha256']
        assert normalize(path,current)==pinned
    return True
if __name__=='__main__':verify();print('PASS exact complete bonfire interaction source amendment;deployment/client outcomes separate')
