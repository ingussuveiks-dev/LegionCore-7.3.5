$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RockawayRelease.ps1"
$out="$repo/build-extractors/runtime-test/spirit-healer-release"
New-Item -ItemType Directory -Force $out | Out-Null
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual game final-link input'}
$gameLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
$source="$repo/src/server/game/Handlers/NPCHandler.cpp"
if((Get-Item $source).LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc -or (Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical spirit healer implementation is stale'}
$run=(Get-Content "$repo/build-extractors/runtime-test/corpse-restart-native/latest-run.txt").Trim()
$inputs=Get-Content "$run/native-inputs.json" -Raw | ConvertFrom-Json
if($inputs.'canonical-worldserver.exe' -ne (Get-FileHash $exe).Hash -or $inputs.'actual-final-link-game.lib' -ne (Get-FileHash $gameLib).Hash -or $inputs.'src/server/game/Handlers/NPCHandler.cpp' -ne (Get-FileHash $source).Hash){throw 'Native spirit run and canonical source/library/executable differ'}
$lines=Get-Content "$run/native-lines.txt"
$abi=@($lines | Where-Object {$_ -match '^SPIRIT_ABI death=\d+$'} | Select-Object -Unique)
if($abi.Count -ne 1 -or @($lines | Where-Object {$_ -match '^SPIRIT_HEALER '}).Count -ne 78 -or @($lines | Where-Object {$_ -match '^SPIRIT_HEALER .* allowed=1$'}).Count -ne 18){throw 'Run all78 native spirit healer cases first'}
$deathOffset=[int]($abi[0].Split('=')[1]);$hex=$deathOffset.ToString('X')
$resurrect=Resolve (Symbol '?ResurrectPlayer@Player@@')
$durability=Resolve (Symbol '?DurabilityLossAll@Player@@')
foreach($symbol in @('?HandleSpiritHealerActivate@WorldSession@@','?SendSpiritResurrect@WorldSession@@')){
 $range=Bounds $symbol;$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
 if($LASTEXITCODE){throw 'Spirit healer disassembly failed'}
 $name=$symbol.Split('@')[0].TrimStart('?');$listing | Set-Content "$out/$name.txt"
 $ops=@(foreach($line in $listing){$m=[regex]::Match($line,'^\s*([0-9A-Fa-f]{16}):\s+(\w+)\s*(.*)$');if($m.Success){[pscustomobject]@{address=[Convert]::ToInt64($m.Groups[1].Value,16);op=$m.Groups[2].Value;args=$m.Groups[3].Value}}})
 $compares=@(for($i=0;$i -lt $ops.Count;$i++){if($ops[$i].op -eq 'cmp' -and $ops[$i].args -match "^dword ptr \[(r\w+)\+0*${hex}h\],0$"){$i}})
 if(!$compares.Count){throw "Missing native ALIVE compare in $name"}
 $index=$compares[0]
 if($index -lt 2 -or $ops[$index+1].op -ne 'je' -or $ops[$index-1].op -ne 'je' -or $ops[$index-2].op -ne 'test' -or $ops[$index-1].args -ne $ops[$index+1].args){throw "Review null/alive guard code generation in $name"}
 if(@($ops[0..$index] | Where-Object {$_.op -in @('call','jmp')}).Count){throw "Side effects precede alive guard in $name"}
 $target=[Convert]::ToInt64($ops[$index+1].args,16)
 $tail=@($ops | Where-Object {$_.address -ge $target})
 $ret=@(for($i=0;$i -lt $tail.Count;$i++){if($tail[$i].op -eq 'ret'){$i}})
 if(!$tail.Count -or $tail[0].address -ne $target -or !$ret.Count -or @($tail[0..$ret[0]] | Where-Object {$_.op -notin @('mov','add','pop','ret','nop')}).Count){throw "Alive branch does not lead directly to side-effect-free epilogue in $name"}
 $calls=@(foreach($op in $ops){if($op.op -eq 'call' -and $op.args -match '^[0-9A-Fa-f]{16}$'){Resolve ([Convert]::ToInt64($op.args,16))}})
 if($calls -notcontains $resurrect -or $calls -notcontains $durability){throw "Native resurrection/durability implementation missing from $name"}
}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;source=(Get-FileHash $source).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;death_offset=$deathOffset;cases=78;accepted=18;alive_guards=2} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Release packet handler and direct spirit resurrection helper reject ALIVE before side effects; native78-case source/library/executable inputs match.'
