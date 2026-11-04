param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-smoke-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$source=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "81c43cc8c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned gate predecessor missing'}
 $marker='class spell_kingslayers_smoke_bomb : public SpellScriptLoader';$body=Block $source $marker;$prior=Block $old $marker
 if(($source.Replace($body,$prior)-replace '\s','') -cne ($old-replace '\s','')){throw 'Only smoke SpellScript may change'}
 [IO.File]::WriteAllText("$out/SmokeAI.inc",(Block $body 'class spell_kingslayers_smoke_bomb_SpellScript : public SpellScript')+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/KingslayersSmokeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE
 Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Smoke ASAN exit=$code or stderr"}
}
