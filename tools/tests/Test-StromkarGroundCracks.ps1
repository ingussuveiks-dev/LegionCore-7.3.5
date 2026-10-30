#requires -Version 7.0
param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-ground-cracks/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block($s,$a){$i=$s.IndexOf($a);if($i -lt 0){throw "Missing $a"};$j=$s.IndexOf('{',$i)+1;$depth=1;while($depth -and $j -lt $s.Length){if($s[$j] -eq '{'){++$depth};if($s[$j] -eq '}'){--$depth};++$j};if($depth){throw "Unclosed $a"};$s.Substring($i,$j-$i)}
 $types=Get-Content "$repo/src/server/game/DataStores/DB2Structure.h" -Raw
 [IO.File]::WriteAllText("$out/GroundTypes.inc",(Block $types 'struct DecalPropertiesEntry')+';'+(Block $types 'struct TextureBlendSetEntry')+';')
 $at=Get-Content "$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeDurationFields.inc",(Block $at '    if (_duration > 0)'))
 [IO.File]::WriteAllText("$out/NativeExpiration.inc",(Block $at '    if (GetDuration() != -1)'))
 $create=Block $at 'bool AreaTrigger::CreateAreaTrigger('
 if(!($create.IndexOf('_ai->CalculateDuration(_duration);') -lt $create.IndexOf('if (_duration > 0)') -and $create.IndexOf('if (_duration > 0)') -lt $create.IndexOf('GetMap()->AddToMap(this)'))){throw 'Native duration serialization no longer precedes initial map creation'}
 foreach($pair in @(@('src/server/game/DataStores/StromkarGroundCrackData.h','GroundData.inc'),@('src/server/scripts/Legion/ClassHalls/stromkar_ground_cracks.h','GroundController.inc'))){$s=Get-Content "$repo/$($pair[0])" -Raw;[IO.File]::WriteAllText("$out/$($pair[1])",([regex]::Replace($s,'(?m)^#(?:include|pragma)[^\r\n]*','')))}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/StromkarGroundCracksTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Production ground projection policy/controller ASAN failed'}
 [IO.File]::WriteAllText("$out/awakening.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /fsanitize=address /Zi `"$PSScriptRoot/StromkarAwakeningTest.cpp`" /Fe`"$out/awakening.exe`" /Fo`"$out/awakening.obj`" /Fd`"$out/awakening.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/awakening.exe`"`n")
 & "$out/awakening.cmd";if($LASTEXITCODE){throw 'Retained actual awakening/recoil ASAN failed'}
 foreach($scope in @('World','Hotfix')){
  & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -Database $scope -EvidenceDirectory "$out/$scope-reader" -Test {
   $key="${scope}DatabaseInfo";$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match "^$key\s*="}).Split('"')[1].Split(';')
   $file=if($scope -eq 'World'){'2026_10_30_31_stromkar_projected_cracks.sql'}else{'2026_10_30_12_stromkar_projected_cracks.sql'}
   $folder=if($scope -eq 'World'){'world'}else{'hotfixes'}
   $path="$repo/sql/pending/$folder/$file";if(!(Test-Path $path)){$path="$repo/sql/updates/$folder/$file"}
   $migration=Get-Content $path -Raw
   $tables=if($scope -eq 'World'){@('areatrigger_template','areatrigger_scripts','areatrigger_actions','areatrigger_data','areatrigger_polygon','areatrigger_template_circle','areatrigger_template_sequence')}else{@('decal_properties','texture_blend_set','hotfix_data')}
   $sql='';$reset='';$snapshot=''
   foreach($table in $tables){$sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table;CREATE TEMPORARY TABLE $table LIKE saved_$table;`n";$reset+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n";$snapshot+="SELECT 'TAG-$table',$table.* FROM $table ORDER BY 2,3;`n"}
   # Remove only this exact new scope in TEMP tables to replay the absent state.
   $absent=if($scope -eq 'World'){"DELETE FROM areatrigger_template WHERE customEntry=900001;DELETE FROM areatrigger_scripts WHERE entry=900001;"}else{"DELETE FROM decal_properties WHERE ID=10000;DELETE FROM texture_blend_set WHERE ID=10000;DELETE FROM hotfix_data WHERE RecordID=10000 AND TableHash IN(3350717493,2512539351);"}
   $sql+="$reset`n$absent`n$migration`n$($snapshot.Replace('TAG','first'))`n$migration`n$($snapshot.Replace('TAG','second'))`n"
   $variants=if($scope -eq 'World'){@("UPDATE areatrigger_template SET Radius=1 WHERE customEntry=900001;","UPDATE areatrigger_template SET HasAttached=1 WHERE customEntry=900001;","DELETE FROM areatrigger_scripts WHERE entry=900001;","UPDATE areatrigger_scripts SET ScriptName='custom' WHERE entry=900001;","INSERT INTO areatrigger_actions(entry,customEntry,id,comment) VALUES(0,900001,0,'counterfactual custom action');","DELETE FROM areatrigger_template WHERE customEntry=900001;")}else{@("UPDATE decal_properties SET Gain=3 WHERE ID=10000;","UPDATE texture_blend_set SET TextureFileDataID1=1346462 WHERE ID=10000;","UPDATE texture_blend_set SET TextureScaleV3=2 WHERE ID=10000;","UPDATE hotfix_data SET Deleted=1 WHERE RecordID=10000 AND TableHash=3350717493;","DELETE FROM hotfix_data WHERE RecordID=10000 AND TableHash=3350717493;","DELETE FROM decal_properties WHERE ID=10000;","UPDATE hotfix_data SET TableHash=3350717493 WHERE RecordID=10000 AND TableHash=2512539351;")}
   $i=0;foreach($variant in $variants){++$i;$sql+="$reset`n$absent`n$migration`n$variant`n$($snapshot.Replace('TAG',"refuse-$i-before"))`n$migration`n$($snapshot.Replace('TAG',"refuse-$i-after"))`n"}
   if($VerifyLiveData){$sql+="$reset`n$($snapshot.Replace('TAG','live-before'))`n$migration`n$($snapshot.Replace('TAG','live-after'))`n"}
   [IO.File]::WriteAllText("$out/$scope.sql",$sql)
   $prior=$env:MYSQL_PWD
   try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names','--force') -InputFile "$out/$scope.sql" -OutputFile "$out/$scope.tsv" -ErrorFile "$out/$scope.stderr"}finally{$env:MYSQL_PWD=$prior}
   $errors=[IO.File]::ReadAllLines("$out/$scope.stderr");$expected=$variants.Count
   if(@($errors|Where-Object{$_ -match '^ERROR 1644 .*Stromkar projected crack inputs differ'}).Count -ne $expected -or @($errors|Where-Object{$_ -match '^ERROR ' -and $_ -notmatch '^ERROR 1644 .*Stromkar projected crack inputs differ'}).Count){throw "Unexpected $scope SQL error; private full evidence retained"}
   $rows=[IO.File]::ReadAllLines("$out/$scope.tsv")
   $groups=@{}
   foreach($line in $rows){$tab=$line.IndexOf("`t");if($tab -lt 1){continue};$tag=$line.Substring(0,$tab);if(!$groups.ContainsKey($tag)){$groups[$tag]=[Collections.Generic.List[string]]::new()};$groups[$tag].Add($line.Substring($tab+1))}
   function Snapshot($tag){if(!$groups.ContainsKey($tag)){return ''};[string]::Join("`n",$groups[$tag])}
   foreach($table in $tables){if((Snapshot "first-$table") -cne (Snapshot "second-$table")){throw "Non-idempotent $table"};for($i=1;$i -le $variants.Count;++$i){if((Snapshot "refuse-$i-before-$table") -cne (Snapshot "refuse-$i-after-$table")){throw "Refused $table/$i mutated"}};if($VerifyLiveData -and (Snapshot "live-before-$table") -cne (Snapshot "live-after-$table")){throw "Live $table differs from deployed projection"}}
   "PASS $scope restricted TEMP absent/applied/idempotence and $expected refusal variants; persistent DML denied"
  }
 }
 if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarGroundCrackCatalog.cpp -ProbeName StromkarGroundCrackCatalog -OutputFile "$out/native.tsv";if($LASTEXITCODE){throw 'Native ground catalog failed'};Get-Content "$out/native.tsv" -Tail 3}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_ground_cracks_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Linked ground projection failed'}}finally{$env:PYTHONPATH=$prior}}
}
