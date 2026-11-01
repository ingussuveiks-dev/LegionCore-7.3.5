param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/quest-cancellation-receipt")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
 $includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
 # Compile the actual value methods locally too: never mix STL-annotated
 # ASAN containers with the uninstrumented full Release library graph.
 $guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
 $values=@();foreach($pattern in @('ObjectGuid::ObjectGuid\(\): _low\(0\), _high\(0\) \{ \}',
  '(?s)void ObjectGuid::SetRawValue\(uint64 high, uint64 low\)\s*\{.*?\n\}',
  '(?s)bool ObjectGuid::operator==\(ObjectGuid const& guid\) const\s*\{.*?\n\}')){
  $match=[regex]::Match($guid,$pattern);if(!$match.Success){throw 'Actual native GUID value method missing'};$values+=$match.Value
 }
 [IO.File]::WriteAllText("$out/NativeGuidValues.inc",($values -join "`n"))
 $hashes=@{};foreach($inputPath in @($PSCommandPath,"$PSScriptRoot/QuestCancellationReceiptTest.cpp","$repo/src/server/game/Entities/Object/QuestObjectScope.h","$repo/src/server/game/Entities/Object/ObjectGuid.cpp","$out/NativeGuidValues.inc")){$hashes[$inputPath]=(Get-FileHash -LiteralPath $inputPath).Hash}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $args=@('/nologo','/EHsc','/std:c++20','/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB','/fsanitize=address','/Zi')
 $args+=($includes+@('C:/local/boost_1_85_0',$out)|ForEach-Object {'/I"'+$_+'"'})
 $args+='"'+$PSScriptRoot+'/QuestCancellationReceiptTest.cpp"'
 $args+=@('/Fe"'+$out+'/test.exe"','/Fo"'+$out+'/test.obj"','/Fd"'+$out+'/test.pdb"','/link','/INCREMENTAL:NO')
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`nset `"PATH=$repo/build-extractors/bin/Release;%PATH%`"`ncl "+($args -join ' ')+"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE
 @{exit=$code;stdout=(Get-FileHash "$out/stdout.log").Hash;stderr=(Get-FileHash "$out/stderr.log").Hash}|ConvertTo-Json|Set-Content "$out/process-result.json"
 Get-Content "$out/stdout.log" -Tail 6
 if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Native cancellation receipt exit=$code or stderr"}
 foreach($inputPath in $hashes.Keys){if((Get-FileHash -LiteralPath $inputPath).Hash -ne $hashes[$inputPath]){throw 'Cancellation receipt input changed during execution'}}
 @{inputs=$hashes;executable=(Get-FileHash "$out/test.exe").Hash;actual_header_asan=$true;full_core_asan=$false}|ConvertTo-Json -Depth 4|Set-Content "$out/input-manifest.json"
 'PASS actual header ASAN/exit/stdout/stderr/post-test diagnostic checks'
}
