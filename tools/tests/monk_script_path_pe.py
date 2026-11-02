"""Exact176-byte native folded script-path lookup leaf, without unwind data."""
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM
from sharasdal_erunak_entry_pe import fields,values

def script_path_lookup(n):
    address=n['symbol']('?GetPathScript@WaypointMgr@@')
    assert address==n['symbol']('?GetConversationData@ConversationDataStoreMgr@@QEBAPEBV?$vector@UConversationData@@')
    end=min(value for value in n['symbols'].values() if value>address)
    assert end-address==176,('Selected folded native lookup must retain its exact public-symbol extent',end-address)
    decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
    body=list(decoder.disasm(n['binary'][n['offset'](address):n['offset'](address)+176],address))
    assert body[-1].address+body[-1].size==end
    assert not any(op.mnemonic in ('call','push','pop','enter','leave') or 'rsp' in op.op_str for op in body)
    assert any(op.mnemonic=='ret' for op in body)
    for op in body:
        if op.mnemonic.startswith('j'):
            assert op.operands[0].type==CS_OP_IMM and address<=op.operands[0].imm<end
    assert {0x48,0x50,0x58,0x70}<=fields(body)
    pairs=[(op.mnemonic,op.op_str) for op in body]
    assert pairs[:3]==[('cmp','qword ptr [rcx + 0x50], 0'),('mov','r9, rcx'),('mov','r8d, edx')]
    assert {0x100000001b3,0xcbf29ce484222325}<={value & 0xffffffffffffffff for value in values(body)}
    assert {('cmp','r8d, dword ptr [rax + 0x10]'),('add','rax, 0x18'),
            ('cmove','rax, rdx'),('and','rcx, qword ptr [r9 + 0x70]'),
            ('add','rcx, qword ptr [r9 + 0x58]')}<=set(pairs)
    return body
