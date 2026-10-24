param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-intro-bubble"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_59_warswords_intro_bubble.sql" -Raw
$tables=@('creature','creature_template','creature_template_addon','creature_addon')
$sql=''
foreach($table in $tables){
 $predicate=switch($table){'creature'{'id=98602'}'creature_template'{'entry=98602'}'creature_template_addon'{'entry=98602'}'creature_addon'{'guid=361284'}}
 $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $predicate;CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
}
$sql+="INSERT IGNORE INTO saved_creature_addon(guid,auras) VALUES(361284,'12345');`n"
$reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$sql+="$reset`nSELECT 'missing',COUNT(*) FROM creature_addon WHERE guid=361283;`n$migration`nSELECT 'first',guid,path_id,mount,bytes1,bytes2,emote,auras FROM creature_addon ORDER BY guid;`n$migration`nSELECT 'second',guid,path_id,mount,bytes1,bytes2,emote,auras FROM creature_addon ORDER BY guid;`n"
foreach($table in @('creature','creature_template','creature_template_addon')){$sql+="SELECT '$table-before',current.* FROM saved_$table current ORDER BY 1,2;SELECT '$table-after',current.* FROM $table current ORDER BY 1,2;`n"}
$variants=[ordered]@{
 'existing-addon'="INSERT INTO creature_addon(guid,mount,auras) VALUES(361283,123,'205424 67890');"
 'template-addon'="INSERT INTO creature_template_addon(entry,auras) VALUES(98602,'67890');"
 'custom-script'="UPDATE creature_template SET ScriptName='custom';"
 'custom-ai'="UPDATE creature_template SET AIName='SmartAI';"
 'custom-phase'="UPDATE creature SET PhaseId='7001 7003';"
 'custom-map'='UPDATE creature SET map=1220;'
 'custom-position'='UPDATE creature SET position_z=100;'
 'custom-mask'='UPDATE creature SET phaseMask=2;'
 'custom-model'='UPDATE creature SET modelid=123;'
 'custom-movement'='UPDATE creature SET MovementType=1;'
 'custom-difficulty'='UPDATE creature SET spawnMask=1;'
 'other-spawn'='UPDATE creature SET guid=361282;'
 'extra-spawn'='INSERT INTO creature(guid,id,map) VALUES(999999999,98602,1511);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',current.* FROM creature_addon current ORDER BY guid;`n$migration`nSELECT '$name-after',current.* FROM creature_addon current ORDER BY guid;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$statement){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($statement|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Warswords temporary bubble fixture failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
function Signature([string]$label){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}))}
if($rows -notcontains "missing`t0" -or $rows -notcontains "first`t361283`t0`t0`t0`t1`t0`t205424"){throw 'Missing or incorrect initial bubble addon'}
foreach($pair in @(@('first','second'))+@($variants.Keys|ForEach-Object {,@("$_-before","$_-after")})+@('creature','creature_template','creature_template_addon'|ForEach-Object {,@("$_-before","$_-after")})){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Bubble migration changes custom/unrelated data: $pair"}}
if($rows -notcontains "first`t361284`t0`t0`t0`t0`t0`t12345"){throw 'Unrelated addon changed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -BubbleBaseline;if(!$? -or $LASTEXITCODE){throw 'Pinned bubble reset baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1";if(!$? -or $LASTEXITCODE){throw 'Bubble reset lifecycle regression failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_59_warswords_intro_bubble.sql" -Algorithm SHA1).Hash.ToLower()
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_59_warswords_intro_bubble.sql';SELECT 'addon',guid,path_id,mount,bytes1,bytes2,emote,auras FROM creature_addon WHERE guid=361283;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "addon`t361283`t0`t0`t0`t1`t0`t205424"){throw 'Bubble migration not deployed'}
 $config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
 $originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
 $nativeOut="$out/native-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
 New-Item -ItemType Directory -Force $nativeOut|Out-Null
 "Native bubble evidence: $nativeOut"
 $config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$nativeOut.Replace('\','/')+'"'))
 [IO.File]::WriteAllText("$nativeOut/worldserver.conf",$config)
 $previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL
 try{
  $env:LEGION_DIALOGUE_CONFIG="$nativeOut/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`""
  Push-Location "$repo/build-extractors/bin/Release"
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/WarswordsIntroBubbleNative.cpp -ProbeName WarswordsIntroBubbleNative -OutputFile "$nativeOut/native.tsv" *> "$nativeOut/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native bubble addon execution failed'}}finally{Pop-Location}
 }finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
 if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed'}
 if((Test-Path "$nativeOut/DBErrors.log") -and (Get-Item "$nativeOut/DBErrors.log").Length){throw 'Native bubble DBErrors require review'}
 Get-Content "$nativeOut/native.tsv"|Where-Object {$_ -match '^(BUBBLE|PASS)'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_intro_bubble_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical bubble reset missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: guarded spawn361283-only infinite205424 bubble, idempotent migration and13 custom/unrelated variants; pending versus completed intro Reset tested. No immunity/pose/timing/client fidelity certification.'
