param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-wings"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_05_stormheim_wings_prerequisite.sql" -Raw
$sql=''
if(!$Live){$sql=@"
CREATE TEMPORARY TABLE saved_addon LIKE quest_template_addon;
INSERT INTO saved_addon SELECT * FROM quest_template_addon WHERE ID IN(38413,38414);
CREATE TEMPORARY TABLE quest_template_addon LIKE saved_addon;
INSERT INTO quest_template_addon SELECT * FROM saved_addon;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
UPDATE quest_template_addon SET PrevQuestID=38412 WHERE ID=38413;
$migration
$migration
UPDATE saved_addon SET PrevQuestID=0 WHERE ID=38413;
SELECT 'actual-addon',a.* FROM quest_template_addon a ORDER BY ID;
SELECT 'expected-addon',a.* FROM saved_addon a ORDER BY ID;
"@}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,a.PrevQuestID,a.NextQuestID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=38413;
SELECT 'gate',SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=38413 ORDER BY SourceTypeOrReferenceId,ElseGroup;
SELECT 'faction',ID,AllowableRaces FROM quest_template WHERE ID IN(38412,38618) ORDER BY ID;
'@
if(!$Live){$sql+=@"
DELETE FROM conditions; UPDATE quest_template_addon SET PrevQuestID=12345 WHERE ID=38413;
$migration
SELECT 'custom-prev',PrevQuestID FROM quest_template_addon WHERE ID=38413;
UPDATE quest_template_addon SET PrevQuestID=38412,NextQuestID=12345 WHERE ID=38413;
$migration
SELECT 'custom-next',PrevQuestID FROM quest_template_addon WHERE ID=38413;
UPDATE quest_template_addon SET NextQuestID=39652 WHERE ID=38413;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(19,38413,8,12345);
$migration
SELECT 'custom-condition-prev',PrevQuestID FROM quest_template_addon WHERE ID=38413;
SELECT 'custom-condition-count',COUNT(*) FROM conditions;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Wings gate data query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/gate$(if($Live){'-live'}).tsv"
if($rows -notcontains "quest`t38413`t39845896`t8192`t18446744073709551615`t0`t39652"){throw 'Wings gate definition changed'}
foreach($source in @(19,20)){foreach($branch in @(0,1)){$id=if($branch){38618}else{38412};if($rows -notcontains "gate`t$source`t$branch`t8`t0`t$id`t0"){throw 'Missing faction gate branch'}}}
if(@($rows | Where-Object {$_ -like "gate`t*"}).Count -ne 4){throw 'Extra Wings conditions'}
if(!$Live){$a=@($rows | Where-Object {$_ -like "actual-addon`t*"} | ForEach-Object {$_.Substring(13)});$e=@($rows | Where-Object {$_ -like "expected-addon`t*"} | ForEach-Object {$_.Substring(15)});if(Compare-Object $a $e){throw 'Unrelated addon fields changed'}
foreach($row in @("custom-prev`t12345","custom-next`t38412","custom-condition-prev`t38412","custom-condition-count`t1")){if($rows -notcontains $row){throw 'Custom gate guard failed'}}}
if($Live){'PASS: live Wings acceptance/marker conditions contain both rewarded faction alternatives.'}
else{'PASS: two rewarded faction alternatives for quest acceptance/markers; repeat migration and unrelated/custom addon/condition preservation.'}
