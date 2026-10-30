param([switch]$Baseline,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/db2-sparse-strings"}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if($Baseline -and $VerifyReleaseBinary){throw 'A failing baseline cannot certify Release'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out) -Test {
 function Execute([string]$name,[string]$command){
  $path="$out/$name.cmd";[IO.File]::WriteAllText($path,$command)
  $start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$env:ComSpec;$start.ArgumentList.Add('/d');$start.ArgumentList.Add('/c');$start.ArgumentList.Add($path)
  $start.WorkingDirectory=$out;$start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
  $child=[Diagnostics.Process]::new();$child.StartInfo=$start;[void]$child.Start();$stdout=$child.StandardOutput.ReadToEndAsync();$stderr=$child.StandardError.ReadToEndAsync();$child.WaitForExit()
  $result=@{ExitCode=$child.ExitCode;Stdout=$stdout.GetAwaiter().GetResult();Stderr=$stderr.GetAwaiter().GetResult()};$child.Dispose()
  [IO.File]::WriteAllText("$out/$name.stdout",$result.Stdout);[IO.File]::WriteAllText("$out/$name.stderr",$result.Stderr)
  [IO.File]::WriteAllText("$out/$name.exit.json",(@{ExitCode=$result.ExitCode;Baseline=[bool]$Baseline}|ConvertTo-Json))
  return $result
 }
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
 $includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
 $libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $environment="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`nset PATH=$repo/build-extractors/bin/Release;%PATH%`ncd /d `"$out`"`n"
 $loader="$repo/src/common/DataStores/DB2FileLoader.cpp"
 if($Baseline){
  $text=(& git -C $repo show 'ce391b93a:src/common/DataStores/DB2FileLoader.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned original loader unavailable'}
  if(!$text.Contains('new char[_header->CatalogDataOffset - _dataStart - records *')){throw 'Pinned baseline lacks original physical-span allocation'}
  $loader="$out/DB2FileLoader-baseline.cpp";[IO.File]::WriteAllText($loader,$text)
 }
 $flags=@('/nologo','/EHsc','/std:c++17','/MD','/W4','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB','/fsanitize=address','/Zi')
 # Match the existing uninstrumented libraries' STL ABI. The entire actual
 # DB2FileLoader and test/probe translation units remain ASAN-instrumented.
 $flags+=@('/D_DISABLE_STRING_ANNOTATION','/D_DISABLE_VECTOR_ANNOTATION')
 $flags+=($includes+@("$repo/src/common/Collision/Management","$repo/src/common/Collision","$repo/dep/g3dlite/include","$repo/dep/recastnavigation/Detour/Include",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')|ForEach-Object {'/I"'+$_+'"'})
 $programs=@(@{Name='synthetic';Source="$PSScriptRoot/DB2SparseStringsTest.cpp"})
 if($VerifyNativeCatalog -or $Baseline){$programs+=@{Name='native-scene';Source="$repo/tools/research/DB2SparseSceneCatalog.cpp"}}
 foreach($program in $programs){
  $name=$program.Name
  $arguments=$flags+@(('"'+$program.Source+'"'),('"'+$loader+'"'),('/Fe"'+$out+'/'+$name+'.exe"'),('/Fo"'+$out+'/"'),('/Fd"'+$out+'/'+$name+'-compiler.pdb"'),'/link','/INCREMENTAL:NO')+@($libs|ForEach-Object {'"'+$_+'"'})
  $build=Execute "build-$name" ($environment+'cl '+($arguments -join ' ')+"`nexit /b %errorlevel%`n")
  if($build.ExitCode -ne 0 -or $build.Stderr -or $build.Stdout -match '\berror (C|LNK)\d+'){throw "Instrumented loader build failed: $name"}
 }
 if($Baseline){
  foreach($case in @(@{Name='original-scene-overflow';Exe='native-scene';Args=('"'+$repo+'/build-extractors/bin/Release/dbc/enUS/"');Pattern='AddressSanitizer: heap-buffer-overflow';Detail='AutoProduceStrings'},
                     @{Name='original-locale-hole';Exe='synthetic';Args='hole-only';Pattern='AddressSanitizer';Detail='AutoProduceStrings'},
                     @{Name='original-plain-pointer';Exe='synthetic';Args='plain-only';Pattern='Assertion failed:';Detail='sparse unlocalized field must contain the string pointer'})){
   $run=Execute $case.Name ($environment+"set LEGION_SCENE_BULK_TEST=1`n`"$out/$($case.Exe).exe`" $($case.Args)`nexit /b %errorlevel%`n")
   if($run.ExitCode -eq 0 -or $run.Stderr -notmatch [regex]::Escape($case.Pattern) -or $run.Stderr -notmatch [regex]::Escape($case.Detail)){throw "Original counterfactual did not reproduce its specific failure: $($case.Name)"}
   "EXPECTED FAIL counterfactual $($case.Name),actual exit=$($run.ExitCode); full stdout/stderr retained, never an ordinary passing run"
  }
 }else{
  $run=Execute 'repaired-synthetic' ($environment+"`"$out/synthetic.exe`"`nexit /b %errorlevel%`n")
  if($run.ExitCode -ne 0 -or $run.Stderr -or $run.Stdout -notmatch 'PASS generated WDC1'){throw 'Repaired generated sparse regression failed'}
  $run.Stdout.Trim()
  if($VerifyNativeCatalog){
   $run=Execute 'repaired-native-scene' ($environment+"set LEGION_SCENE_BULK_TEST=1`n`"$out/native-scene.exe`" `"$repo/build-extractors/bin/Release/dbc/enUS/`"`nexit /b %errorlevel%`n")
   if($run.ExitCode -ne 0 -or $run.Stderr -or $run.Stdout -notmatch '(?m)^COMPLETE\t5969\t7883015\r?$'){throw 'Complete native scene materialization/byte comparison failed'}
   $run.Stdout.Trim()
  }
  if($VerifyReleaseBinary){
   $prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_db2_sparse_strings_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical sparse string PE verification failed'}}finally{$env:PYTHONPATH=$prior}
  }
 }
}
