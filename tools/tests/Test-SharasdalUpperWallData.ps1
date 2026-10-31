param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-wall-data"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$prior=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 foreach($name in @('SharasdalUpperWallCatalog','SharasdalUpperWallGeometry','SharasdalHealCatalog')){& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "$name.cpp" -ProbeName $name -OutputFile "$out/$name.tsv";if(!$? -or $LASTEXITCODE){throw "Native $name failed"}}
}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$native=Get-Content "$out/SharasdalUpperWallCatalog.tsv"
foreach($row in @("EFFECT`t209904`t0`t0`t2`t0`t0`t0`t104`t0","TARGET`t209904`t0`t2`t1","MISC`t209904`t0`t1`t0`t1`t0`t0")){if($native -notcontains $row){throw "Native Thud changed: $row"}}
if(@($native|Where-Object {$_ -like "EFFECT`t*`t61`t*`t49819`t*"}).Count){throw 'Review newly discovered native49819 spell producer'}
$criteria=Get-Content "$out/SharasdalHealCatalog.tsv"
foreach($row in @("STEP`t4`t2287`t47132`t0`tDestroy the Sea Witch","TREE`t4`t47133`t47132`t0`t1`t29662`tAsk Grash to destroy the ice wall.","CRITERION`t4`t29662`t92`t49819`t1","TREE`t4`t47380`t47132`t0`t1`t29803`tSlay Lady Zithreen.","CRITERION`t4`t29803`t92`t50397`t1")){if($criteria -notcontains $row){throw "Native final stage changed: $row"}}
$floor=Get-Content "$out/SharasdalUpperWallGeometry.tsv"
if($floor -notcontains "LOAD`t1"){throw 'Native final-wall tile did not load'}
$samples=@($floor|Where-Object {$_ -like "FINAL_FLOOR`t*"});if($samples.Count -ne 9){throw 'Incomplete actual floor footprint'}
foreach($row in $samples){$f=$row.Split("`t");$z=[double]::Parse($f[3],[Globalization.CultureInfo]::InvariantCulture);if([Math]::Abs($z-807.66) -gt 0.2){throw 'Authored final point no longer matches real VMAP floor'}}
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'grash',guid,id,map,position_x,position_y,position_z,PhaseID FROM creature WHERE guid=362092;
SELECT 'wall',guid,id,map,position_x,position_y,position_z,PhaseID,state FROM gameobject WHERE guid=134298;
SELECT 'model',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID=69084;
SELECT 'gossip',MenuID,TextID FROM gossip_menu WHERE MenuID=19467;
SELECT 'spell-script',COUNT(*) FROM spell_script_names WHERE spell_id=209904;
SELECT 'spell-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry=209904;
SELECT 'spell-linked',COUNT(*) FROM spell_linked_spell WHERE ABS(spell_trigger)=209904 OR ABS(spell_effect)=209904;
SELECT 'EOF','sharasdal-upper-wall';
'@
[IO.File]::WriteAllText("$out/read.sql",$query)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $sqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Upper wall full-EOF read-only context failed'}
}finally{$env:MYSQL_PWD=$prior}
$expected=@'
grash	362092	105298	1600	-181.168	801.922	796.974	6120
wall	134298	248514	1600	-10.3385	802.349	806.072	6120	1
model	69084	3.15883	4.5
gossip	19467	28747
spell-script	0
spell-condition	0
spell-linked	0
EOF	sharasdal-upper-wall
'@
if(Compare-Object ($expected.Replace("`r",'') -split "`n") @(Get-Content "$out/world.tsv")){throw 'Review changed live actor/wall/model/gossip or native cast extensions'}
$distance=[Math]::Sqrt([Math]::Pow(-17.96+10.3385,2)+[Math]::Pow(802.05-802.349,2)+[Math]::Pow(807.66-806.072,2))
if($distance -gt 5+4.5){throw 'Native5-yard door search no longer reaches exact final wall with actual Grash combat reach'}
'PASS actual26972/read-onlyHotfix step4/wall29662/49819 and separate Lady29803/50397, ordinary self-range instant cone Thud209904, no native49819 spell producer, exact live actor/wall/model/gossip and nine native VMAP final-pose samples. No live mutation or client traversal claim.'
