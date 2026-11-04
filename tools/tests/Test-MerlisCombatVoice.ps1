param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/merlis-combat-voice/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
 $pinned=(& git -C $repo show 'd8c85e9fe:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned combat source missing'}
 $ai=Block $source 'struct npc_merlis_malaganAI : ScriptedAI';$prior=Block $pinned 'struct npc_merlis_malaganAI : ScriptedAI'
 $start=$ai.IndexOf('        uint32 combatVoiceDelay');$last=Block $ai 'void RequestCombatVoice(';$end=$ai.IndexOf($last,$start)+$last.Length
 if($start -lt 0 -or $end -le $start){throw 'Dedicated combat voice boundary missing'}
 $helpers=$ai.Substring($start,$end-$start);[IO.File]::WriteAllText("$out/CombatVoice.inc",$helpers)
 $normalized=$ai.Remove($start,$end-$start)
 foreach($name in @('void Reset(','void EnterCombat(','void UpdateAI(')){
  $current=Block $ai $name;$before=Block $prior $name;$normalized=$normalized.Replace($current,$before)
  $label=if($name -like '*Reset*'){'Reset'}elseif($name -like '*EnterCombat*'){'Enter'}else{'Update'}
  [IO.File]::WriteAllText("$out/Current$label.inc",$current);[IO.File]::WriteAllText("$out/Before$label.inc",$before)
 }
 if(($source.Replace($ai,$normalized)-replace '\s','') -cne ($pinned-replace '\s','')){throw 'Unreviewed change outside Merlis combat voice helpers/reset/entry/update'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MerlisCombatVoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Combat voice ASAN exit=$code or stderr"}
}
