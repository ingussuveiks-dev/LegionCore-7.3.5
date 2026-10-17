param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-weather"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_03_stormheim_weather_chain.sql" -Raw
$sql=''
if(!$Live){
 $sql+="CREATE TEMPORARY TABLE saved_quest_template_addon AS SELECT * FROM quest_template_addon WHERE ID IN(38312,38318,38405,38410,38612,38613,38614,38615,38558); CREATE TEMPORARY TABLE quest_template_addon AS SELECT * FROM saved_quest_template_addon; UPDATE quest_template_addon SET PrevQuestID=0 WHERE ID=38405; UPDATE quest_template_addon SET ExclusiveGroup=0 WHERE ID IN(38318,38405,38613,38614);`n$migration`n$migration`nUPDATE saved_quest_template_addon SET PrevQuestID=38312 WHERE ID=38405; UPDATE saved_quest_template_addon SET ExclusiveGroup=-38318 WHERE ID IN(38318,38405); UPDATE saved_quest_template_addon SET ExclusiveGroup=-38613 WHERE ID IN(38613,38614); SELECT 'actual',t.* FROM quest_template_addon t; SELECT 'expected',t.* FROM saved_quest_template_addon t;`n"
}
$sql+=@'
SELECT 'chain',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags,a.ExclusiveGroup FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38312,38318,38405,38410,38612,38613,38614,38615) ORDER BY q.ID;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(38405,38614,38410,38615) ORDER BY quest,id;
SELECT 'quest-givers',COUNT(*) FROM creature_template WHERE entry IN(91481,91519,92566,92567) AND (npcflag & 2)=2;
SELECT 'condition-gates',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38405,38614,38410,38615);
'@
if(!$Live){$sql+="UPDATE quest_template_addon SET PrevQuestID=99999,ExclusiveGroup=123 WHERE ID=38405; UPDATE quest_template_addon SET ExclusiveGroup=456 WHERE ID=38614;`n$migration`nSELECT 'custom-previous',PrevQuestID,ExclusiveGroup FROM quest_template_addon WHERE ID=38405; SELECT 'custom-group',ExclusiveGroup FROM quest_template_addon WHERE ID=38614;"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Weather chain query failed'}}finally{$env:MYSQL_PWD=$previous}
if(!$Live){$a=@($rows | Where-Object {$_.StartsWith("actual`t")} | ForEach-Object {$_.Substring(7)} | Sort-Object);$e=@($rows | Where-Object {$_.StartsWith("expected`t")} | ForEach-Object {$_.Substring(9)} | Sort-Object);if(Compare-Object $a $e){throw 'Weather chain complete-row preservation failed'};foreach($line in @("custom-previous`t99999`t123","custom-group`t456")){if($rows -notcontains $line){throw 'Weather chain custom setting overwritten'}}}
foreach($line in @("quest-givers`t4","condition-gates`t0","starter`t91481`t38405","starter`t92566`t38614","starter`t91519`t38410","starter`t92566`t38615","starter`t92567`t38615")){if($rows -notcontains $line){throw "Weather chain source contract missing: $line"}}
$rows | Set-Content "$out/chain$(if($Live){'-live'}).tsv"
'PASS: parallel faction prerequisite migration twice; full addon rows/custom prerequisites/groups and actual quest-giver relations preserved.'
