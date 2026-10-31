"""Selected native dynamic NPC lifetime and admission proof (26972 Release)."""
from capstone import CS_OP_MEM
from sharasdal_erunak_entry_pe import sites


def sequence(body, expected):
    actual = [(x.mnemonic, x.op_str) for x in body]
    matches = [i for i in range(len(actual) - len(expected) + 1)
               if actual[i:i + len(expected)] == expected]
    assert len(matches) == 1, f'Reinspect native NPC lifetime sequence: {expected}'
    return matches[0]


def verify_viewer(n):
    body = n['instructions']('?CanNeverSee2@Conversation@@')
    assert sites(n, body, '?getMSTime@@') == [12], 'Viewer clock must be sampled once'
    assert body[10].op_str == 'byte ptr [rdx + 0x16], 4' and body[11].mnemonic == 'jne'
    existing = sequence(body, [('sub', 'r15d, dword ptr [rbx + 0x2c]'),
                               ('mov', 'edx, 0xc'), ('mov', 'rcx, rsi')])
    assert existing == 45 and sites(n, body, '?GetUInt32Value@Object@@') == [48, 60]
    assert [(x.mnemonic, x.op_str) for x in body[49:51]] == [('cmp', 'r15d, eax'), ('seta', 'al')]
    assert body[51].mnemonic == 'jmp' and body[51].operands[0].imm == body[126].address
    assert body[52].op_str == 'byte ptr [rsi + 0x504], 0' and body[53].mnemonic == 'je'
    assert body[54].op_str == 'dword ptr [rsi + 0x4dc], 0' and body[55].mnemonic == 'je'
    sequence(body, [('mov', 'ebx, r15d'), ('sub', 'ebx, dword ptr [rsi + 0x500]'),
                    ('mov', 'edx, 0xc'), ('mov', 'rcx, rsi')])
    assert body[61].op_str == 'ebx, eax' and body[62].mnemonic == 'ja'
    reject = body[125].address
    assert body[11].operands[0].imm == reject == body[62].operands[0].imm
    assert (body[125].mnemonic, body[125].op_str) == ('mov', 'al, 1')
    assert body[53].operands[0].imm == body[63].address == body[55].operands[0].imm
    assert body[122].op_str == 'dword ptr [rdi + 0x2c], r15d'
    assert body[123].mnemonic == 'xor' and body[123].op_str == 'al, al'
    # Existing keys exit before the sole timestamp store. New-view rejection
    # exits before allocation/insertion; static and explicit-zero duration skip it.
    stores = [i for i,x in enumerate(body) if x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
              and x.operands[0].mem.disp == 0x2c]
    assert stores == [110, 122]
    print('PASS native viewer: Player-only, one clock sample, existing full field12 deadline without refresh, dynamic NPC new-view window before insertion, unchanged static/zero-duration admission')


def verify(n):
    ins = n['instructions']; ctor = ins('??0Conversation@@'); create = ins('?CreateConversation@Conversation@@')
    sequence(ctor, [('mov', 'dword ptr [rdi + 0x500], esi'), ('mov', 'byte ptr [rdi + 0x504], sil')])
    assert ('xor', 'esi, esi') in [(x.mnemonic,x.op_str) for x in ctor[:10]]
    assert sites(n, create, '?GenerateDynamicFieldData@Conversation@@') == [238]
    assert sites(n, create, '?SetUInt32Value@Object@@') == [186, 249]
    stamp = sequence(create, [('mov', 'dword ptr [rdi + 0x500], eax'),
                              ('cmp', 'byte ptr [r14 + 0x16], 4'), ('setne', 'al'),
                              ('mov', 'byte ptr [rdi + 0x504], al')])
    assert stamp == 252 and sites(n, create, '?getMSTime@@') == [251]
    assert create[250].op_str == 'dword ptr [rdi + 0x4dc], ebx'
    assert sites(n, create, '??$AddToMap@VConversation@@@Map@@') == [258]
    body = ins('?Update@Conversation@@')
    assert sites(n, body, '?IsQuestObjectExpired@WorldObject@@') == [3]
    assert body[26].op_str == 'dword ptr [rdi + 0x4dc], 0' and body[27].mnemonic == 'je'
    assert sites(n, body, '?GetUInt32Value@Object@@') == [31] and body[28].op_str == 'edx, 0xc'
    assert body[32].op_str == 'byte ptr [rdi + 0x504], 0' and body[34].mnemonic == 'je'
    assert body[34].operands[0].imm == body[97].address
    assert sites(n, body, '?getMSTime@@') == [35, 104]
    sequence(body, [('mov', 'ecx, eax'), ('mov', 'r8d, eax'),
                    ('sub', 'ecx, dword ptr [rdi + 0x500]'), ('cmp', 'ecx, esi')])
    assert body[40].mnemonic == 'jbe' and body[40].operands[0].imm == body[111].address
    assert body[41].op_str == 'rcx, qword ptr [rdi + 0x4f0]'
    sequence(body, [('mov', 'edx, r8d'), ('sub', 'edx, dword ptr [rcx + 0x2c]'), ('cmp', 'edx, esi')])
    assert body[49].mnemonic == 'jbe' and body[49].operands[0].imm == body[111].address
    assert body[75].mnemonic == 'je' and body[75].operands[0].imm == body[46].address
    # NPC removal is the actual inlined native Remove path, not a guessed call.
    assert sites(n, body, '?IsInWorld@Object@@') == [13, 83]
    assert sites(n, body, '?AddObjectToRemoveList@WorldObject@@') == [20, 90]
    assert body[88].mnemonic == 'call' and body[88].op_str == 'qword ptr [rax + 0x10]'
    sequence(body, [('lea', 'rdx, [rdi + 0x4e0]'), ('mov', 'qword ptr [rsp + 0x30], rbx'),
                    ('lea', 'rcx, [rdi + 0x4f0]')])
    assert body[101].op_str == 'rax, qword ptr [rdi + 0x4f0]' and body[102].mnemonic == 'je'
    assert body[103].op_str == 'ebx, dword ptr [rax + 0x2c]'
    sequence(body, [('sub', 'eax, ebx'), ('cmp', 'eax, esi')])
    assert body[107].mnemonic == 'jbe' and sites(n, body, '?Remove@Conversation@@') == [109]
    static = ins('?LoadConversationFromDB@Conversation@@')
    assert not sites(n, static, '?getMSTime@@')
    assert not any(x.mnemonic.startswith('mov') and x.operands[0].type == CS_OP_MEM
                   and x.operands[0].mem.disp in (0x500, 0x504) for x in static)
    verify_viewer(n)
    print('PASS native NPC lifetime: initialized appended fields, actual post-budget pre-AddToMap stamp, native creation horizon and every admitted viewer deadline, real removal queue, retained Player caster clock/quest-first cleanup and untimed static loader')
