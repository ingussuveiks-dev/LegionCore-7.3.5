param([switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/chen-escort-speech/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed source block'};$s.Substring($a,$b-$a)}
 function Prior([string]$path){$s=(& git -C $repo show "f5122dd06:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned complete pre-spacing source missing'};$s}
 $root='src/server/scripts/Scenario/Artifacts/Monk/Intro';$path="$root/chen_presentation.h";$current=Get-Content "$repo/$path" -Raw;$before=Prior $path
 $normal=$current.Replace((Block $current 'void UpdateEscortSpeech('),'').Replace('uint32 rescueSpokenAt = 0;','').Replace('(rescueIssued & 3) == 3','rescueIssued == 3').Replace('rescueIssued |= 2; return;','rescueIssued = 3; return;').Replace(' rescueSpokenAt = getMSTime();','')
 if((Shape $normal) -cne (Shape $before)){throw 'Only actual speech clock and optional escort speech may change; complete presentation retained'}
 $path="$root/chen_rescue.h";$rescue=Get-Content "$repo/$path" -Raw;$oldRescue=Prior $path
 $oldBlock=Block $oldRescue 'if (!escortSpoken)'
 if((Shape $rescue.Replace('escortSpoken = true;',$oldBlock)) -cne (Shape $oldRescue)){throw 'Only optional Talk5 dispatch may move; all native route/owner/credit behavior retained'}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instance=Get-Content "$repo/$path" -Raw;$oldInstance=Prior $path
 if(([regex]::Matches($instance,[regex]::Escape('chenPresentation.UpdateEscortSpeech(diff); '))).Count -ne 1 -or (Shape $instance.Replace('chenPresentation.UpdateEscortSpeech(diff); ','')) -cne (Shape $oldInstance)){throw 'Only one after-route optional speech update permitted in entire shared instance file'}
 if($ScopeOnly){'PASS exact optional Talk5 relocation/actual Talk4 clock/after-route call; full remaining presentation/rescue/shared instance unchanged';return}
 $text=Get-Content "$PSScriptRoot/ChenEscortSpeechOriginal.json" -Raw|ConvertFrom-Json
 $columns=@($text[0].PSObject.Properties.Name);$pairs=[string]::Join(',',@($columns|ForEach-Object {"'$_',``$_``"}))
 [IO.File]::WriteAllText("$out/world.sql","SET SESSION TRANSACTION READ ONLY;SELECT JSON_OBJECT($pairs) FROM creature_text WHERE CreatureID=103148 AND GroupID IN (4,5) ORDER BY GroupID,ID;SELECT 'EOF';")
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names') -InputFile "$out/world.sql" -OutputFile "$out/world.stdout" -ErrorFile "$out/world.stderr"}finally{$env:MYSQL_PWD=$prior}
 $rows=@(Get-Content "$out/world.stdout");if($code -or (Get-Item "$out/world.stderr").Length -or $rows.Count -ne 3 -or $rows[-1] -cne 'EOF'){throw 'Read-only original full Chen speech context failed'}
 for($i=0;$i -lt 2;++$i){if(($rows[$i]|ConvertFrom-Json|ConvertTo-Json -Compress) -cne ($text[$i]|ConvertTo-Json -Compress)){throw 'Original full Chen reply/escort text row changed'}}
 'PASS read-only full103148/group4/99966 and group5/100187 retained with Duration0/Sound0; no SQL mutation'
 & "$PSScriptRoot/Test-MonkStudentAudio.ps1" -OutputDirectory "$out/audio"
 if(!$?){throw 'Actual native voice/hash/full-decode checks failed'}
 $audio=Get-Content "$PSScriptRoot/MonkStudentRepliesAudio.json" -Raw|ConvertFrom-Json;$voice=@($audio.Payloads|Where-Object FileDataID -eq 1384171)
 if($voice.Count -ne 1 -or $voice[0].VoiceMs -ne 2125){throw 'Actual original Talk4 duration differs'}
 & "$PSScriptRoot/Test-MonkChenEarlyPresentation.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Actual current instance/actor/native dispatch export failed'}
 [IO.File]::WriteAllText("$out/EscortBeforeRescue.inc",$oldRescue.Replace('namespace MonkChenRescue','namespace MonkChenRescueBeforeEscort'))
 [IO.File]::WriteAllText("$out/EscortBeforePresentation.inc",$before.Replace('#include "chen_rescue.h"','').Replace('#include "ScriptedCreature.h"','').Replace('MonkChenRescue::','MonkChenRescueBeforeEscort::').Replace('namespace MonkChenPresentation','namespace MonkChenPresentationBeforeEscort'))
 $old=(Block $oldInstance 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_before_escort').Replace('MonkChenRescue::State','MonkChenRescueBeforeEscort::State').Replace('MonkChenPresentation::State','MonkChenPresentationBeforeEscort::State')
 # Existing Nine/Jorvinax owners retain their current actual implementation;
 # adapt only their pointer constructor types for the full predecessor host.
 foreach($pair in @(@('Jorvinax','MonkJorvinax'),@('NineArrival','MonkNineArrival'),@('NineRescue','MonkNineRescue'))){$body=(Get-Content "$out/$($pair[0]).inc" -Raw).Replace('#include "Rescue.inc"','').Replace('MonkChenRescue::','MonkChenRescueBeforeEscort::').Replace("namespace $($pair[1])","namespace $($pair[1])BeforeEscort");[IO.File]::WriteAllText("$out/EscortBefore$($pair[0]).inc",$body);$old=$old.Replace("$($pair[1])::State","$($pair[1])BeforeEscort::State")}
 [IO.File]::WriteAllText("$out/EscortBeforeInstance.inc",$old+';')
 [IO.File]::WriteAllText("$out/EscortVoiceMs.inc","constexpr uint32 EscortVoiceMs=$($voice[0].VoiceMs);")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ChenEscortSpeechTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";$nativeExit=$LASTEXITCODE;[IO.File]::WriteAllText("$out/native-result.json",(@{Exit=$nativeExit}|ConvertTo-Json));if($nativeExit){throw 'Actual optional speech ASAN regression failed'}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_chen_escort_speech_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual registered Release optional speech proof failed'}}finally{$env:PYTHONPATH=$previous}}
}
