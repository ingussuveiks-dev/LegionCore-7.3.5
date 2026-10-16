param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/action-bar-persistence"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($part in @(@('void Player::_LoadActions(', 'void Player::_LoadAuras(', 'Load.inc'),@('void Player::_SaveActions(', 'void Player::_SaveAuras(', 'Save.inc'),@('ActionButton* Player::AddActionButton(', 'bool Player::AddSpellToActionBarIfAppropriate(', 'Add.inc'),@('void Player::RemoveActionButton(', 'bool Player::UpdatePosition(', 'Remove.inc'))){
 $a=$source.IndexOf($part[0]);$b=$source.IndexOf($part[1],$a)
 if($a -lt 0 -or $b -le $a){throw "Native action section missing: $($part[0])"}
 [IO.File]::WriteAllText("$out/$($part[2])",$source.Substring($a,$b-$a))
}
$a=$source.IndexOf('void Player::ActivateTalentGroup(');$b=$source.IndexOf('    SendActionButtons(1);',$a)
$activation=$source.Substring($a,$b-$a)
if(!$activation.Contains('_LoadActions(CharacterDatabase.Query(stmt), true);')){throw 'Specialization must load/merge even an empty query result'}
$header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
$a=$header.IndexOf('enum ActionButtonUpdateState');$b=$header.IndexOf('struct PlayerCreateInfoItem',$a)
[IO.File]::WriteAllText("$out/Types.inc",$header.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ActionBarPersistenceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native action persistence regression failed'}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$binary.Contains('Action bar: preserved learned actions while loading specialization %u')){throw 'Canonical Release lacks specialization action merge'}
}
'PASS: extracted production action states, load/add/remove/save; empty and existing specialization layouts, pending auto additions, invalid old spells, 64-bit macro IDs, save/relog, repeated merges, deletions and spec isolation. Client verification pending.'
