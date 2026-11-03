"""Certify pose-owned DOOR collision in the actual linked Release, not a copy hash."""
from pathlib import Path
import runpy,subprocess
from sharasdal_erunak_entry_pe import sites
repo=Path(__file__).resolve().parents[2]
path='src/server/game/Entities/GameObject/GameObject.cpp'
before=subprocess.check_output(['git','-C',str(repo),'show','a9e86af7b:'+path],text=True,encoding='utf-8')
current=(repo/path).read_text(encoding='utf-8')
addition='''    // Door collision follows its physical pose in SetGoState. Applying the
    // generic loot rule here re-blocks open doors and unblocks closed resets.
    if (GetGoType() == GAMEOBJECT_TYPE_DOOR)
        return;
'''
assert current.count(addition)==1 and current.replace(addition,'')==before,'Shared engine certificate: only DOOR collision exception may change'
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
body=n['instructions']('?SetLootState@GameObject@@')
pairs=lambda rows:[(x.mnemonic,x.op_str) for x in rows]
callback,=sites(n,body,'?OnGameObjectLootStateChanged@ScriptMgr@@')
kind,pose=sites(n,body,'?GetByteValue@Object@@')
assert callback<kind<pose
assert pairs(body[kind-3:kind])==[('mov','edx, 0x18'),('mov','r8b, 1'),('mov','rcx, rdi')]
assert pairs(body[kind+1:kind+3])==[('test','al, al'),('je',body[kind+2].op_str)]
target=body[kind+2].operands[0].imm
tail=[x for x in body if x.address>=target]
assert tail and all(x.mnemonic in ('mov','add','pop','ret') for x in tail)
assert not any(x.mnemonic=='mov' and x.op_str.startswith('byte ptr') for x in tail),'DOOR must skip all generic model writes'
assert ('mov','byte ptr [rcx + 0x28], bl') in pairs(body[pose:]),'Other GO generic collision rule missing'
physical=n['instructions']('?SetGoState@GameObject@@')
assert [('cmp','edi, 1'),('sete','al'),('mov','byte ptr [rcx + 0x28], al')] == pairs(physical)[pairs(physical).index(('cmp','edi, 1')):pairs(physical).index(('cmp','edi, 1'))+3]
print('PASS actual SetLootState keeps AI/global callbacks then returns for type0 DOOR before generic collision writes; SetGoState retains closed READY collision, other GO loot logic retained. Real4413 native regression is separate from client traversal.')
