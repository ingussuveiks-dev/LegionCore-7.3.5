param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/scripted-scene-recipient"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
function Commands([string]$s){$a=$s.IndexOf('            case SCRIPT_COMMAND_PLAYSCENE:');$b=$s.IndexOf('            case SCRIPT_COMMAND_SUMMON_CONVERSATION:',$a);if($a -lt 0 -or $b -le $a){throw 'Scene command block missing'};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo) -Test {
 $s=Get-Content "$repo/src/server/game/Scripting/MapScripts.cpp" -Raw;$current=Commands $s
 [IO.File]::WriteAllText("$out/Recipient.inc",(Block $s 'inline Player* Map::_GetScriptPlayer('))
 [IO.File]::WriteAllText("$out/SendScene.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::SendSpellScene('))
 [IO.File]::WriteAllText("$out/SendEvent.inc",(Block (Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw) 'void Spell::EffectSendEvent('))
 $old=(& git -C $repo show '94438634a:src/server/game/Scripting/MapScripts.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned scene baseline missing'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 foreach($mode in @('baseline','current')){
  $defines=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''};if($mode -eq 'baseline'){$defines+=' /DBASELINE'}
  [IO.File]::WriteAllText("$out/Commands.inc",$(if($mode -eq 'baseline'){Commands $old}else{$current}))
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $defines /I`"$out`" `"$PSScriptRoot/ScriptedSceneRecipientTest.cpp`" /Fe:$mode.exe /Fo:$mode.obj /Fd:$mode.pdb`nif errorlevel 1 exit /b 1`n$mode.exe`nexit /b %errorlevel%`n")
  & "$out/run.cmd";if($LASTEXITCODE){throw "Scene recipient $mode failed"}
 }
}
