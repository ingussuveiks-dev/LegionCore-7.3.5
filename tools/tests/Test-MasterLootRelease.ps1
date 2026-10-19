$ErrorActionPreference='Stop'
# Preserve the prior shared slot/view/AutoStore checks against this executable.
. "$PSScriptRoot/Test-LostArmyRelease.ps1"
foreach($path in @('Groups/Group.cpp','Handlers/LootHandler.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Final-link game library predates $path"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/master-loot-native'
New-Item -ItemType Directory -Force $out | Out-Null
$checks=@(@('?HandleMasterLootItem@WorldSession@@','handler',1),@('?GroupLoot@Group@@','group-loot',2),@('?DoRollForAllMembers@Group@@','manual-roll',1),@('?CountTheRoll@Group@@','winner',1))
$verified=@{}
foreach($check in $checks){
 $range=Bounds $check[0]; $argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $lines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
 if($LASTEXITCODE){throw "Canonical $($check[1]) disassembly failed"}
 $lines | Set-Content "$out/$($check[1])-disassembly.txt"
 $sites=0
 for($i=0;$i -lt $lines.Count;$i++){
  $call=[regex]::Match($lines[$i],'call\s+([0-9A-Fa-f]+)\s*$')
  if(!$call.Success -or (Resolve ([Convert]::ToInt64($call.Groups[1].Value,16))) -ne $allowed){continue}
  $prefix=$lines[[Math]::Max(0,$i-16)..$i] -join "`n"
  # x64: this/player/item/currency in RCX/RDX/R8D/R9D; type,
  # needs_quest and LootItem* occupy stack20h/28h/30h. Native fixture
  # static assertions and quest-bit check validate these member offsets.
  $pointer=[regex]::Match($prefix,'mov\s+qword ptr \[rsp\+30h\],(r[a-z0-9]+)\s*\r?\n')
  if(!$pointer.Success){throw "Missing actual LootItem pointer argument in $($check[1])"}
  $base=[regex]::Escape($pointer.Groups[1].Value)
  foreach($pattern in @("mov\s+r9d,dword ptr \[$base\+4\]","mov\s+r8d,dword ptr \[$base\]","movzx\s+[a-z0-9]+,byte ptr \[$base\+38h\]",'mov\s+byte ptr \[rsp\+20h\],[a-z]+','mov\s+byte ptr \[rsp\+28h\],[a-z]+',"movzx\s+[a-z0-9]+,byte ptr \[$base\+64h\]",'shr\s+al,6','and\s+al,1')){
   if($prefix -notmatch $pattern){throw "Canonical $($check[1]) lacks native recipient argument setup: $pattern"}
  }
  $sites++
 }
 if($sites -ne $check[2]){throw "Wrong recipient call count for $($check[1]): $sites"}
 $verified[$check[1]]=$sites
}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;recipient_call_sites=$verified} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release supplies actual CurrencyID/type/quest flag/LootItem pointer at all5 native MasterLoot/GroupLoot/winner recipient calls; final-link freshness and hashes recorded.'
