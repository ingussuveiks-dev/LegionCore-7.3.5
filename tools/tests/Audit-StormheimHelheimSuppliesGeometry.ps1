$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimSuppliesData.ps1" -Live
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-helheim-supplies"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$source.IndexOf('bool GameObject::IsInRange(');$b=$source.IndexOf('void GameObject::EventInform(',$a)
if($a -lt 0 -or $b -lt 0){throw 'Missing native GO range handler'}
[IO.File]::WriteAllText("$output/NativeBoneRange.inc",$source.Substring($a,$b-$a).Replace('GameObject::','BoneInteractionObject::'))
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previousPassword=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@("SELECT COUNT(*) FROM gameobject_display_info WHERE ID=6328;" | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE -or $rows.Count -ne 1 -or $rows[0] -ne '0'){throw 'Unexpected native bone display override'}}finally{$env:MYSQL_PWD=$previousPassword}
$rows | Set-Content "$output/bone-display-hotfix.txt"
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimStemTideGeometry','HelheimBoneInteractionGeometry')){
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"'
$compilerArgs+='"'+$repo+'/tools/tests/'+$test+'.cpp"'
$compilerArgs+='/Fe"'+$output+'/'+$test+'.exe"';$compilerArgs+='/Fo"'+$output+'/'+$test+'.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw "Native $test build failed"}
}
$results=@()
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/StormheimStemTideGeometry.exe" --self-test
 if($LASTEXITCODE){throw 'Incomplete path status self-test failed'}
 foreach($row in Get-Content "$output/positions-live.tsv"){
  $fields=$row.Split("`t")
  @('366.422 369.746 27.5921',($fields[2..4] -join ' ')) | Set-Content "$output/ground.txt"
  $ground=@(& "$output/StormheimStemTideGeometry.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 65535 1463);$groundExit=$LASTEXITCODE
  # Preserve the original ground-only failures. The independent player route
  # admits ordinary water and still rejects magma/slime, partial and exhausted paths.
  $center=@(& "$output/StormheimStemTideGeometry.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 8192 1463 walk-swim);$centerExit=$LASTEXITCODE
  Add-Content "$output/ground.txt" $fields[5]
  $result=@(& "$output/HelheimBoneInteractionGeometry.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 8192 1463 walk-swim);$code=$LASTEXITCODE
  $results+=[pscustomobject]@{spawn=[int]$fields[0];position=@($fields[2..4] | ForEach-Object {[double]$_});orientation=[double]$fields[5];ground_connected=($groundExit -eq 0);ground_exit=$groundExit;ground_evidence=$ground;center_connected=($centerExit -eq 0);center_exit=$centerExit;center_evidence=$center;connected=($code -eq 0);native_exit=$code;evidence=$result}
  $result | Select-Object -First 4
 }
 $results | ConvertTo-Json -Depth 5 | Set-Content "$output/geometry-player.json"
 $failedGround=@($results | Where-Object {!$_.ground_connected})
 if(($failedGround.spawn -join ',') -ne '127694,127702,127704' -or @($failedGround | Where-Object {$_.ground_exit -ne 6}).Count){throw 'Ground-only baseline changed; retain and inspect native statuses'}
 $failedCenter=@($results | Where-Object {!$_.center_connected})
 if(($failedCenter.spawn -join ',') -ne '127694,127704'){throw 'Conservative center baseline changed; inspect geometry-player.json'}
 if(@($results | Where-Object {!$_.connected}).Count){throw 'Native GO interaction approach incomplete; inspect geometry-player.json'}
 foreach($resolved in $failedGround){if(!($resolved.evidence -match 'terrain polygons ground=\d+ water=[1-9]\d* magma/slime=0') -or !($resolved.evidence -match 'endpoint .* staticLOS=1 PASS')){throw 'Missing native water/surface/LOS evidence'}}
}finally{$env:PATH=$previousPath}
'PASS:16/16 native GO walking/swimming interaction approaches. Original13/16 ground-only and14/16 center-distance baselines retained. No spawn/navmesh/runtime change; client still pending.'
