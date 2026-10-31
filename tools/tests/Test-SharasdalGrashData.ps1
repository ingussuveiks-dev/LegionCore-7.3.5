param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-grash-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$saved=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalHealCatalog.cpp -ProbeName SharasdalHealCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Matching native Sharasdal catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$saved}
$catalog=Get-Content "$out/native.tsv"
foreach($row in @(
 "STEP`t0`t2282`t47120`t0`tBig Help",
 "TREE`t0`t47121`t47120`t0`t1`t29658`tHeal the Sea Giant.",
 "TREE`t0`t47161`t47120`t0`t2`t29677`tKill the Naga Brutes.",
 "TREE`t0`t47180`t47120`t0`t1`t29688`t[Invisible] Intro Events Complete",
 "TREE`t0`t47303`t47120`t0`t1`t29763`tRecruit the Sea Giant.",
 "CRITERION`t0`t29658`t92`t49815`t1","CRITERION`t0`t29677`t92`t49861`t2",
 "CRITERION`t0`t29688`t92`t49900`t1","CRITERION`t0`t29763`t92`t50161`t1",
 "CRITERION`t2`t29660`t92`t49817`t1","CRITERION`t4`t29662`t92`t49819`t1"
)){if($catalog -notcontains $row){throw "Matching native criterion changed: $row"}}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName,npcflag,gossip_menu_id FROM creature_template WHERE entry=105298;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'menu',MenuID,OptionID,OptionNpc,BoxCoded,BoxMoney FROM gossip_menu_option WHERE MenuID IN(19509,19483,19467) ORDER BY MenuID,OptionID;
SELECT 'text',MenuID,TextID FROM gossip_menu WHERE MenuID IN(19509,19483,19467) ORDER BY MenuID;
SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id IN(10993808,10993809,10993810) GROUP BY id;
SELECT 'criteria-extra',criteria_id,type,value1,value2,ScriptName FROM achievement_criteria_data WHERE criteria_id IN(29658,29677,29688,29763,29660,29662) ORDER BY criteria_id,type;
SELECT 'EOF','sharasdal-grash-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$saved=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $sqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Grash full-EOF read-only context failed'}
}finally{$env:MYSQL_PWD=$saved}
$expected=@("binding`t105298`tSmartAI`tnpc_grash`t0`t19509","scenario`t1066`t1600`t0`t7",
 "menu`t19467`t0`t0`t0`t0","menu`t19483`t0`t0`t0`t0","menu`t19509`t0`t0`t0`t0",
 "text`t19467`t28747","text`t19483`t28782","text`t19509`t28831",
 "path`t10993808`t13`t1`t13","path`t10993809`t11`t1`t11","path`t10993810`t6`t1`t7","EOF`tsharasdal-grash-data")
if(Compare-Object $expected @(Get-Content "$out/world.tsv")){throw 'Review changed Grash binding/menu/paths or criteria extra admission rows against matching native data'}
'PASS native26972+Hotfix heal/intro/recruit separate type92 leaves, both preserved later wall criteria; exact World binding/class/flags/three menus/texts/path counts, no extra criterion conditions and full EOF/stderr readback. No SQL written or retail choreography/client proof claimed'
