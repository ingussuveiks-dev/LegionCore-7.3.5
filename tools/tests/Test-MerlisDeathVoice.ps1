param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/merlis-death-voice/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
 . "$PSScriptRoot/AltheaActorIdentityCertificate.ps1"
 $source=Restore-AltheaActorIdentityBoundary $source
 $pinned=(& git -C $repo show '3ca15b909:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned original Merlis death source missing'}
 $actor=Block $source 'class DeathConversation final : public Conversation'
 [IO.File]::WriteAllText("$out/MerlisDeathActor.inc",$actor+';')
 $normalized=$source.Replace($actor+';','').Replace('new DeathConversation(me->GetGUID())','new Conversation')
 # Comments do not affect the whole-file implementation boundary.
 if((($normalized-replace '//[^\n]*','')-replace '\s','') -cne (($pinned-replace '//[^\n]*','')-replace '\s','')){throw 'Only nested Merlis death resolver and2448 allocation may change'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MerlisDeathActorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Merlis death actor ASAN exit=$code or stderr"}
}
