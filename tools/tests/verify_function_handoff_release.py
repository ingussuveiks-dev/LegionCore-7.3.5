"""Actual linked queue-lock admission, with pinned whole-source scope proof."""
from pathlib import Path
import hashlib,re,subprocess
reader=Path(__file__).with_name('verify_pending_ai_release.py');source=reader.read_text();ctx={'__file__':str(reader)};exec(compile(source[:source.index('plan=json.loads')],str(reader),'exec'),ctx)
n=ctx['n'];ins=ctx['instructions'];root=reader.resolve().parents[2]
path='src/common/Utilities/FunctionProcessor.cpp'
old=subprocess.check_output(['git','-C',str(root),'show','b08be202a:'+path]).decode().replace('\r\n','\n')
current=(root/path).read_text()
before='''    if (m_functions_queue.empty())
        return;

    FunctionList tempFunctions;
    {
        std::lock_guard<std::recursive_mutex> _queue_lock(m_queue_lock);
        std::swap(tempFunctions, m_functions_queue);
    }
'''
after='''    // Producers insert under this same lock; even empty() reads shared
    // multimap state and must not race a network-thread insertion.
    std::unique_lock<std::recursive_mutex> queueLock(m_queue_lock);
    if (m_functions_queue.empty())
        return;

    FunctionList tempFunctions;
    std::swap(tempFunctions, m_functions_queue);
    queueLock.unlock();
'''
assert old.count(before)==1 and old.replace(before,after)==current,'shared scheduler change exceeds pinned lock scope'
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
def imported(name):
    rows=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})\s+f',text)
    assert len(rows)==1
    return n['resolve'](int(rows[0],16))
lock=imported('_Mtx_lock');unlock=imported('_Mtx_unlock')
rows=ins('?AddFunctionsFromQueue@FunctionProcessor@@')
def sites(target):return [i for i,x in enumerate(rows) if x.mnemonic=='call' and x.op_str.startswith('0x') and n['resolve'](int(x.op_str,16))==target]
acquire,=sites(lock);releases=sites(unlock)
assert rows[acquire-1].mnemonic=='mov' and rows[acquire-1].op_str=='rcx, r15','wrong mutex call argument'
assert any(x.mnemonic=='lea' and x.op_str=='r15, [rcx + 0x28]' for x in rows[:acquire]),'wrong scheduler mutex'
reads=[i for i,x in enumerate(rows) if '[rdi + 0x20]' in x.op_str]
assert len(reads)==1 and acquire<reads[0]<min(releases),'pending multimap size read outside lock'
assert rows[reads[0]].mnemonic=='cmp' and rows[reads[0]+1].mnemonic=='je','empty/nonempty split missing'
empty=int(rows[reads[0]+1].op_str,16)
empty_at=next(i for i,x in enumerate(rows) if x.address==empty)
swap=next(i for i,x in enumerate(rows) if x.mnemonic=='lea' and x.op_str=='rdx, [rdi + 0x18]')
assert len(releases)==2 and reads[0]<swap<releases[0]<empty_at<releases[1],'empty and swapped paths must both unlock'
assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rdx], rcx' for x in rows[swap:releases[0]]),'pending map swap missing'
assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rdx + 8], rcx' for x in rows[swap:releases[0]]),'pending size swap missing'
assert rows[empty_at].mnemonic=='test' and rows[empty_at].op_str=='r12b, r12b','empty path lost RAII lock state'
assert rows[releases[0]+1].op_str=='r12b, r12b','nonempty path does not retire lock state before iteration'
assert len([x for x in rows[:reads[0]] if x.mnemonic=='call'])==1,'empty admission unexpectedly allocates temporary map'
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',text);assert stamp and int(stamp[1],16)==n['u32'](n['pe']+8)
print('PASS linked FunctionProcessor producer mutex precedes pending empty read and swap; both paths unlock; pinned whole-source change only moves read under existing lock; SHA256='+hashlib.sha256(n['binary']).hexdigest().upper())
