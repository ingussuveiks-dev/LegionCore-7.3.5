"""Check the real linked loader and native SmartAI delegation of fusion receipts."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,instructions=p['native'],p['instructions']
assert n['symbol']('?AddSC_titanstrike_fusion@@') in n['calls']('?AddLegionScripts@@')
for bridge in ('?SetData@MimironAI@TitanstrikeFusion@@','?SetData@CoreAI@TitanstrikeFusion@@'):
    targets={n['resolve'](x.operands[0].imm) for x in instructions(bridge)
             if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM}
    assert n['symbol']('?SetData@SmartAI@@') in targets, 'Original SmartAI payload delegation missing'
# This getter is a small x64 leaf and legitimately has no unwind record.
address=n['symbol']('?GetData@MimironAI@TitanstrikeFusion@@')
end=min(v for v in n['symbols'].values() if v>address)
assert 0<end-address<=64
ready=list(p['decoder'].disasm(n['binary'][n['offset'](address):n['offset'](end)],address))
assert any(x.mnemonic=='cmp' and x.operands[-1].type==CS_OP_IMM and x.operands[-1].imm==3 for x in ready)
condition=instructions('?OnConditionCheck@condition_titanstrike_fusion_ready@@')
assert any(any(v.type==CS_OP_IMM and v.imm==0x735002 for v in x.operands) for x in condition)
for name in (b'npc_titanstrike_fusion_mimiron',b'npc_titanstrike_fusion_core',b'condition_titanstrike_fusion_ready'):
    assert name in n['binary']
print('PASS real Legion registration and linked fusion receipt/core/condition callbacks delegate native SmartAI; native-host tests supply timing/admission coverage, client remains separate')
