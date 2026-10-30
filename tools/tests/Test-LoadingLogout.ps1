param([switch]$BaselineOnly,[string]$BaselineRef='d54eeebbe',[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/loading-logout"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Section([string]$source,[string]$start,[string]$end){$a=$source.IndexOf($start);$b=$source.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Native logout section missing: $start"};$source.Substring($a,$b-$a)}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 foreach($kind in @('baseline')+$(if(!$BaselineOnly){@('repaired')}else{@()})){
  $target="$out/$kind";New-Item -ItemType Directory -Force $target|Out-Null
  $sources=@{};foreach($path in @('Maps/Map.cpp','Server/WorldSession.cpp','Server/WorldSession.h','Handlers/CharacterHandler.cpp')){
   if($kind -eq 'baseline'){$sources[$path]=(& git -C $repo show "${BaselineRef}:src/server/game/$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned native baseline'}}else{$sources[$path]=Get-Content "$repo/src/server/game/$path" -Raw}
  }
  [IO.File]::WriteAllText("$target/MapIteration.inc",(Section $sources['Maps/Map.cpp'] '    // update worldsessions for existing players' '    uint32 _ms = GetMSTimeDiffToNow(_s);'))
  [IO.File]::WriteAllText("$target/Schedule.inc",(Section $sources['Server/WorldSession.cpp'] '        if (canLogout) // Logout only if remove from map' '        ///- Cleanup socket pointer if need'))
  [IO.File]::WriteAllText("$target/LogoutRequest.inc",(Section $sources['Handlers/CharacterHandler.cpp'] 'void WorldSession::HandleLogoutRequest(' 'void WorldSession::HandleLogoutInstant('))
  $should=Section $sources['Server/WorldSession.h'] '        bool ShouldLogOut(time_t currTime) const' '        void LogoutPlayer(bool Save);'
  $a=$should.IndexOf('{');$b=$should.LastIndexOf('}');[IO.File]::WriteAllText("$target/ShouldLogOut.inc",$should.Substring($a+1,$b-$a-1))
  $define=if($kind -eq 'baseline'){'/DCOUNTERFACTUAL'}else{''}
  [IO.File]::WriteAllText("$target/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $define /I`"$target`" `"$PSScriptRoot/LoadingLogoutTest.cpp`" /Fe`"$target/test.exe`" /Fo`"$target/test.obj`" /Fd`"$target/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$target/test.exe`"`nexit /b %errorlevel%`n")
  & "$target/run.cmd";if($LASTEXITCODE){throw "Native loading/logout $kind regression failed"}
 }
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
  $previous=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_loading_logout_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual loading/logout Release verification failed'}}finally{$env:PYTHONPATH=$previous}
 }
}
