"""Exact new amendment, independent of historical quest scope certificates."""
from pathlib import Path
import io,json,subprocess
from five_ten_20261111_scope import retained_source as before_five_ten
REPO=Path(__file__).resolve().parents[2]
PLAN=json.loads((REPO/'tools/tests/HealingWidthChanges.json').read_text())
_retained={}
def pinned_sources(commit,paths):
    result=subprocess.run(['git','-C',str(REPO),'cat-file','--batch'],input=''.join(commit+':'+p+'\n'for p in paths).encode(),capture_output=True,check=True)
    stream=io.BytesIO(result.stdout);blobs={}
    for path in paths:
        header=stream.readline().split()
        if header[-1]==b'missing':
            assert path=='src/server/game/Spells/HealingAmount.h'
            blobs[path]=b'';continue
        assert len(header)==3 and header[1]==b'blob'
        blobs[path]=stream.read(int(header[2]));assert stream.read(1)==b'\n'
    assert not stream.read()
    return blobs
def verify():
    if _retained:return
    originals=pinned_sources(PLAN['baseline'],[row['path']for row in PLAN['changes']])
    for row in PLAN['changes']:
        old=originals[row['path']].decode(row['encoding']).replace('\r\n','\n')
        lines=old.splitlines(True)
        for edit in reversed(row['edits']):
            at=edit['line'];before=edit['old'].splitlines(True)
            assert lines[at:at+len(before)]==before,(row['path'],at)
            lines[at:at+len(before)]=edit['new'].splitlines(True)
        actual=before_five_ten(row['path'],(REPO/row['path']).read_bytes().decode(row['encoding']).replace('\r\n','\n'))
        assert ''.join(lines)==actual,'Undeclared healing-width amendment:'+row['path']
        _retained[row['path']]=old
def retained_source(path):
    verify()
    return _retained[path] if path in _retained else before_five_ten(path)
if __name__=='__main__':
    verify()
    print('PASS exact unsigned64 healing amendment:',len(_retained),'files; historical scope is checked against pinned4d61 sources separately from actual native/ASAN tests')
