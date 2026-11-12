"""Selected compiler output checks complement full-method ASAN and native calls."""
from five_ten_20261111_pe import ins,sites,values,n
from second_ten_ten_20261112_scope import require_fixed
require_fixed()
rows=ins('?Finish@PetBattle@@')
assert any(x.mnemonic=='cmp'and x.op_str=='eax, 2'and rows[i+1].mnemonic=='je'and rows[i+2].mnemonic=='cmp'and rows[i+2].op_str=='eax, 3'and rows[i+3].mnemonic=='je'and rows[i+1].op_str==rows[i+3].op_str for i,x in enumerate(rows[:-3])),'Both terminal statuses return before completion side effects'
assert not sites(rows,'?GetAvailablesPets@PetBattleTeam@@'),'XP admission must not reuse temporary swap admission'
assert any(x.mnemonic=='cmp'and x.op_str=='byte ptr [rsp + 0x4c], 0'and rows[i+1].mnemonic=='jne'for i,x in enumerate(rows[:-1])),'True ignore-abandon flag skips penalty'
assert any(x.mnemonic=='sub'and x.op_str=='rsi, rcx'for x in rows)and any(x.mnemonic=='cmp'and x.op_str=='r9d, 0x19'for x in rows),'Native64 XP remainder and level25 cap are linked'
assert sum(x.mnemonic=='mulss'for x in rows)>=5 and any(x.mnemonic=='jb'and x.op_str.startswith('0x')and int(x.op_str,16)<x.address for x in rows),'Native XP threshold recalculation/back edge remains'
rows=ins('?CreateRequest@PetBattleSystem@@');assert len(sites(rows,'?RemoveRequest@PetBattleSystem@@'))==1,'Owning request replacement releases previous allocation'
rows=ins('?HandlePetBattleInput@WorldSession@@');assert any(x.mnemonic=='test'and x.op_str=='r8d, r8d'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1]))and sites(rows,'?ForfeitBattle@PetBattleSystem@@'),'Zero action reaches compiled leave switch'
rows=ins('?Update@PetBattleSystem@@');assert sum(x.mnemonic=='mov'and x.op_str.endswith('+ 0x1c], 0xffffffff')for x in rows)==2,'New proposals reset both native signed pending answers'
for method in ['HandlePetBattleRequestPVP','HandlePetBattleRequestUpdate']:
 assert len(sites(ins('?'+method+'@WorldSession@@'),'?CanPlayerEnterInPetBattle@PetBattleSystem@@'))==1,'Real duel request/acceptance routes run shared admission'
rows=ins('?CanPlayerEnterInPetBattle@PetBattleSystem@@');assert any(x.mnemonic=='mov'and x.op_str=='rcx, rsi'and rows[i+1].mnemonic=='call'for i,x in enumerate(rows[:-1])),'Admission retains actual initiating player receiver'
print('PASS linked second-hundred first10: terminal completion guards,swap-independent XP,native remainder/threshold loop,forfeit/input,request ownership,fresh consent,and both actual duel admission routes;ASAN/native/client scopes separate')
