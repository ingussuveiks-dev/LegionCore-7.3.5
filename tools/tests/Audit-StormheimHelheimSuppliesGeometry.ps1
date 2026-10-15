$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimSuppliesData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-helheim-supplies"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimJudgmentGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/HelheimSuppliesGround.exe"';$compilerArgs+='/Fo"'+$output+'/HelheimSuppliesGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Regal native ground build failed'}
$results=@()
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 foreach($row in Get-Content "$output/positions-fixture.tsv"){
  $fields=$row.Split("`t")
  @('366.422 369.746 27.5921',($fields[2..4] -join ' ')) | Set-Content "$output/ground.txt"
  $result=& "$output/HelheimSuppliesGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 1463
  $results+=[pscustomobject]@{spawn=[int]$fields[0];position=@($fields[2..4] | ForEach-Object {[double]$_});connected=($LASTEXITCODE -eq 0);native_exit=$LASTEXITCODE;evidence=$result}
  $result
 }
}finally{$env:PATH=$previousPath}
$results | ConvertTo-Json -Depth 4 | Set-Content "$output/geometry.json"
$unresolved=@($results | Where-Object {!$_.connected})
"Native geometry survey: $($results.Count-$unresolved.Count)/$($results.Count) connected; unresolved spawns: $($unresolved.spawn -join ', '). These failures are retained as evidence; no position repair is inferred from navmesh alone."
