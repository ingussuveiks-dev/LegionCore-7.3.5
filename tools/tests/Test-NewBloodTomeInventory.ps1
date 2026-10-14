$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-tome-inventory"
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Export-Section($file,$start,$end,$name) {
 $source=Get-Content "$repo/$file" -Raw
 $a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -lt $a){throw "Production section missing: $start"}
 [IO.File]::WriteAllText("$output/$name",$source.Substring($a,$b-$a))
}
Export-Section 'src/server/game/Entities/Player/Player.h' 'enum PlayerSlots' 'struct ItemPosCount' 'InventorySlots.inc'
Export-Section 'src/server/game/Entities/Item/Container/Bag.cpp' 'uint32 Bag::GetItemCount(' 'uint32 Bag::GetItemCountWithLimitCategory(' 'BagCount.inc'
Export-Section 'src/server/game/Entities/Player/Player.cpp' 'void Player::DestroyItemCount(uint32 item,' 'void Player::DestroyZoneLimitedItem(' 'DestroyCount.inc'
Export-Section 'src/server/game/Entities/Player/Player.cpp' 'uint32 Player::GetItemCount(uint32 item,' 'uint32 Player::GetItemCountWithLimitCategory(' 'PlayerCount.inc'
Export-Section 'src/server/game/Entities/Player/Player.cpp' '    // Match inventory to a fresh prison restart' '    if (IsVoidStorageUnlocked())' 'InventoryRestore.inc'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/NewBloodTomeInventoryTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'New Blood native tome inventory regression failed'}
