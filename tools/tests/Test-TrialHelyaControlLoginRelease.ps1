$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaControlRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-control-native'
if((Get-Item "$repo/src/server/game/Spells/SpellMgr.cpp").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw 'Final-link game library predates Bleak save policy'}
$range=Bounds '?LoadSpellCustomAttr@SpellMgr@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$attrs=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical spell-custom-attribute disassembly failed'}
$attrs | Set-Content "$out/spell-custom-attr-disassembly.txt"
$indexes=@(for($i=0;$i -lt $attrs.Count;$i++){if($attrs[$i] -match 'mov\s+dword ptr \[rsp\+[^\]]+\],389B6h'){$i}})
if($indexes.Count -ne 1){throw 'Canonical Bleak spell-fix initializer missing/ambiguous'}
$index=$indexes[0];$region=$attrs[$index..($index+7)] -join "`n"
$array=@([regex]::Matches($region,'lea\s+rax,\[rsp\+([0-9A-Fa-f]+)h\]'))
if($array.Count -ne 2 -or ([Convert]::ToInt64($array[1].Groups[1].Value,16)-[Convert]::ToInt64($array[0].Groups[1].Value,16)) -ne 4){throw 'Bleak policy is not scoped to a single spell ID'}
$callback=[regex]::Match($region,'lea\s+rdx,\[([0-9A-Fa-f]{16})h\]')
if(!$callback.Success){throw 'Missing native Bleak spell-fix callback'}
$call=[regex]::Match($attrs[$index+7],'call\s+([0-9A-Fa-f]{16})')
if(!$call.Success -or (Resolve ([Convert]::ToInt64($call.Groups[1].Value,16))) -ne (Resolve (Symbol '?ApplySpellFix@@'))){throw 'Bleak initializer does not invoke the native spell-fix dispatcher'}
$address=Resolve ([Convert]::ToInt64($callback.Groups[1].Value,16));$offset=Offset $address
# Exact leaf callback reached by the one-ID initializer: OR AttributesCu[0]
# with CANT_BE_SAVED_IN_DB0x800, then return. Fail on ABI/codegen changes.
$bytes=($binary[$offset..($offset+7)] | ForEach-Object {$_.ToString('X2')}) -join ''
if($bytes -ne '81490400080000C3'){throw 'Canonical Bleak callback lacks scoped no-save flag; review ABI if changed'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;spell_mgr_source=(Get-FileHash "$repo/src/server/game/Spells/SpellMgr.cpp").Hash;spell=231862;single_id_initializer=$true;native_no_save_callback=$true} | ConvertTo-Json | Set-Content "$out/control-login-release-inputs.json"
'PASS: canonical custom-attribute initializer applies no-save0x800 only to Bleak231862 through its native callback.'
