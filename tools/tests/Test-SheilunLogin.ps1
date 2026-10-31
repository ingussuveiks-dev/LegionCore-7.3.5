param([switch]$Baseline,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-login$(if($Baseline){'-baseline'})"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$token){$a=$s.IndexOf($token);if($a -lt 0){throw "Missing $token"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $token"};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw
[IO.File]::WriteAllText("$out/Recovery.inc",(Block $source 'bool Player::ExitCompletedSheilunLogin()'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/Inventory.inc",(Block $source 'Item* Player::GetItemByEntry('))
[IO.File]::WriteAllText("$out/Objective.inc",(Block $source 'int32 Player::GetQuestObjectiveData(uint32'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
$enums=@('PlayerSlots','ChildEquipmentSlots','EquipmentSlots','InventorySlots','InventoryPackSlots','BankItemSlots','BankBagSlots','ReagentSlots')|ForEach-Object {(Block $source "enum $_")+';'}
[IO.File]::WriteAllText("$out/Slots.inc",($enums -join "`n"))
$source=if($Baseline){(& git -C $repo show 'fa1beeac4:src/server/game/Handlers/CharacterHandler.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Handlers/CharacterHandler.cpp" -Raw}
if($Baseline -and $LASTEXITCODE){throw 'Pinned baseline caller missing'}
$a=$source.IndexOf('        bool addedToLoginMap =');$b=$source.IndexOf('        sObjectAccessor->AddObject(player);',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native post-AddPlayer login block missing'}
[IO.File]::WriteAllText("$out/Admission.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" /I`"$repo/src/server/game/Scenarios`" `"$PSScriptRoot/SheilunLoginTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Sheilun rejected-login recovery regression failed'}
