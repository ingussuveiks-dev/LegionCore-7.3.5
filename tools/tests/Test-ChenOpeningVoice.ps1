param([switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/chen-opening-voice/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $paths=@('src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp','src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_presentation.h')
 $changes=@(@('if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(4443,','if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(4000,'),@('elapsed >= 4443','elapsed >= 4000'))
 for($i=0;$i -lt 2;++$i){$current=Get-Content "$repo/$($paths[$i])" -Raw;$old=(& git -C $repo show "8b9ea7da4:$($paths[$i])") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-voice source missing'};if(([regex]::Matches($current,[regex]::Escape($changes[$i][0]))).Count -ne 1 -or (Shape $current.Replace($changes[$i][0],$changes[$i][1])) -cne (Shape $old)){throw 'Only the shared Chen reply deadline may change; full actor/header retained'}}
 if($ScopeOnly){'PASS exact two native4000->4443 reply deadlines; all other actor/state/timing/route/credit behavior unchanged';return}
 $context=Get-Content "$PSScriptRoot/ChenOpeningVoiceOriginal.json" -Raw|ConvertFrom-Json
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 foreach($table in $context.tables.PSObject.Properties.Name){
  $columns=@($context.tables.$table[0].PSObject.Properties.Name);$pairs=[string]::Join(',',@($columns|ForEach-Object {"'$_',``$_``"}));$stem="$out/world-$table"
  [IO.File]::WriteAllText("$stem.sql","SET SESSION TRANSACTION READ ONLY;SELECT JSON_OBJECT($pairs) FROM $table WHERE $($context.filters.$table) ORDER BY $($context.orders.$table);SELECT 'EOF';")
  $prior=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names','--default-character-set=utf8mb4') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"}finally{$env:MYSQL_PWD=$prior}
  $rows=@(Get-Content "$stem.stdout");if($exit -or (Get-Item "$stem.stderr").Length -or $rows[-1] -cne 'EOF' -or $rows.Count -ne @($context.tables.$table).Count+1){throw 'Chen native World context read failed'}
  for($i=0;$i -lt $rows.Count-1;++$i){$actual=$rows[$i]|ConvertFrom-Json|ConvertTo-Json -Depth 12 -Compress;$wanted=$context.tables.$table[$i]|ConvertTo-Json -Depth 12 -Compress;if($actual -cne $wanted){throw "Chen full World row changed: $table"}}
 }
 'PASS fresh read-only native103148/group3/99965 duration0/no sound override and complete original template; no SQL change'
 & "$PSScriptRoot/Test-MonkStudentAudio.ps1" -OutputDirectory "$out/audio"
 if(!$?){throw 'Fresh actual native Chen voice/hash/full decode failed'}
 $audio=Get-Content "$PSScriptRoot/MonkStudentRepliesAudio.json" -Raw|ConvertFrom-Json;$voice=@($audio.Payloads|Where-Object FileDataID -eq 1384167)
 if($voice.Count -ne 1 -or $voice[0].VoiceMs -ne 4443){throw 'Actual native first voice differs'}
 & "$PSScriptRoot/Test-MonkChenEarlyPresentation.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current actual actor/instance/native-clock export failed'}
 $header=(& git -C $repo show '8b9ea7da4:src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_presentation.h') -join "`n";if($LASTEXITCODE){throw 'Pinned full pre-voice controller missing'}
 [IO.File]::WriteAllText("$out/VoiceBaseline.inc",$header.Replace('#include "chen_rescue.h"','#include "Rescue.inc"').Replace('#include "ScriptedCreature.h"','').Replace('namespace MonkChenPresentation','namespace MonkChenPresentationBeforeVoice'))
 [IO.File]::WriteAllText("$out/VoiceMs.inc","constexpr uint32 NativeVoiceMs=$($voice[0].VoiceMs);")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ChenOpeningVoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";$nativeExit=$LASTEXITCODE;[IO.File]::WriteAllText("$out/native-result.json",(@{Exit=$nativeExit}|ConvertTo-Json));if($nativeExit){throw 'Chen voice/native early regression failed'}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_chen_opening_voice_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Chen voice Release check failed'}}finally{$env:PYTHONPATH=$previous}}
}
