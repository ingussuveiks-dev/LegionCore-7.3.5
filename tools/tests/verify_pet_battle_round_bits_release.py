"""Prove the real native round writer flushes its three-bit death count."""
from five_ten_20261111_pe import n,ins,sites
import hashlib,json
body=ins('??6@YAAEAVByteBuffer@@AEAV0@AEBURoundResult@BattlePet@WorldPackets@@@Z')
pairs=lambda rows:[(x.mnemonic,x.op_str)for x in rows]
bits,=sites(body,'?WriteBits@ByteBuffer@@')
assert {('mov','rdx, qword ptr [r15 + 0x48]'),('sub','rdx, qword ptr [r15 + 0x40]'),('mov','r8d, 3'),('mov','rcx, rsi')}<=set(pairs(body[bits-5:bits]))
flush=body[bits+1:bits+10]
assert pairs(flush[:1])==[('cmp','qword ptr [rsi + 0x18], 8')]
assert flush[1].mnemonic=='je'
assert {('mov','r8d, 1'),('mov','qword ptr [rsi + 0x18], 8'),('lea','rdx, [rsi + 0x20]'),('mov','rcx, rsi'),('mov','byte ptr [rsi + 0x20], 0')}<=set(pairs(flush))
assert len(sites(flush,'?append@ByteBuffer@@'))==1,'Actual partial death-count byte must be appended'
assert flush[1].operands[0].imm==body[bits+9].address,'The already-aligned branch must rejoin after the flush'
assert pairs(body[bits+9:bits+12])==[('mov','r12, qword ptr [r15 + 0x18]'),('mov','r14, qword ptr [r15 + 0x10]'),('cmp','r14, r12')],'Flush must precede the empty-effects branch,including a completely empty tail'
print('PASS actual linked RoundResult native3-bit death count and immediate byte append/alignment before both optional tails; real writer/wire ASAN regression and client scopes remain separate')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_death_count_bits':3,'empty_tail_flush':True}))
