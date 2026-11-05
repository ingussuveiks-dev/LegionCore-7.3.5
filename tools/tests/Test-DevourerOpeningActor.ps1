param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-opening-actor/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
 . "$PSScriptRoot/DevourerOpeningCertificate.ps1"
 [void](Restore-DevourerOpeningBoundary $source)
 [IO.File]::WriteAllText("$out/DevourerOpeningActor.inc",(Block $source 'class OpeningConversation final : public Conversation')+';')
 $scope=Get-Content "$repo/src/server/game/Entities/Object/QuestObjectScope.h" -Raw
 [IO.File]::WriteAllText("$out/QuestScope.inc",(Block $scope 'class QuestCancellationReceipt')+';'+(Block $scope "`nstruct QuestObjectScope")+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/DevourerOpeningActorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Devourer death actor ASAN exit=$code or stderr"}
}
