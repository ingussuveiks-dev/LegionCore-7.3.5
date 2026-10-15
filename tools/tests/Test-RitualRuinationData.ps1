$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_24_ritual_ruination.sql" -Raw
$filters=@{
 smart_scripts='entryorguid IN(110479,110533,110548,110556,252054,109956)'
 conditions='(SourceTypeOrReferenceId=23 AND SourceGroup=7543 AND SourceEntry IN(1,9,10,11)) OR (SourceTypeOrReferenceId=15 AND SourceGroup=110533)'
 gameobject_template='entry IN(252054,249742)'
 creature_template='entry IN(110548,110533)'
 spell_script_names='spell_id IN(219917,219839,219920,219923,220374)'
}
$fixture=''
foreach($table in $filters.Keys){
 $fixture+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$select=@'
SELECT 'state',JSON_OBJECT(
 'chestLoot',(SELECT Data1 FROM gameobject_template WHERE entry=252054),
 'chestScript',(SELECT ScriptName FROM gameobject_template WHERE entry=252054),
 'imageScript',(SELECT ScriptName FROM creature_template WHERE entry=110548),
 'oldBat',(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=110479 AND source_type=0 AND id IN(0,1)),
 'oldPickup',(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=252054 AND source_type=1 AND id IN(0,1)),
 'bindings',(SELECT COUNT(*) FROM spell_script_names WHERE ScriptName LIKE 'spell_ritual_%'),
 'duelReset',(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=110556 AND source_type=0 AND id=8),
 'lootExists',(SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=252054 AND Item=128941 AND Chance=100),
 'unrelatedChest',(SELECT a.Data1=b.Data1 AND a.ScriptName=b.ScriptName FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry=249742),
 'previousScenario',(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=109956)=(SELECT COUNT(*) FROM saved_smart_scripts WHERE entryorguid=109956),
 'introQuest',(SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=110533 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=43254),
 'badLinks',(SELECT COUNT(*) FROM smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.link=b.id AND b.event_type=61 WHERE a.link<>0 AND b.id IS NULL));
SELECT 'phase',JSON_OBJECT('entry',SourceEntry,'objective',ConditionValue2,'negative',NegativeCondition)
 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7543 AND SourceEntry IN(9,10) AND ConditionTypeOrReference=41 ORDER BY SourceEntry,ConditionValue2;
'@
$sql=$fixture+"`n"+$migration+"`nSELECT 'PASS1';`n"+$select+"`n"+$migration+"`nSELECT 'PASS2';`n"+$select
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Ritual Ruination temporary SQL replay failed'}
}finally{$env:MYSQL_PWD=$old}
$states=@($rows | Where-Object {$_ -like "state`t*"} | ForEach-Object {($_ -split "`t",2)[1] | ConvertFrom-Json})
if($states.Count -ne 2){throw 'Both migration applications must be checked'}
foreach($s in $states){
 if($s.chestLoot -ne 252054 -or $s.chestScript -ne 'go_ritual_scepter' -or $s.imageScript -ne 'npc_ritual_guldan_image' -or
    [int]$s.oldBat -ne 0 -or [int]$s.oldPickup -ne 0 -or [int]$s.bindings -ne 5 -or [int]$s.duelReset -ne 0 -or [int]$s.badLinks -ne 0 -or [int]$s.lootExists -ne 1 -or [int]$s.introQuest -ne 1 -or !$s.unrelatedChest -or !$s.previousScenario){throw "Ritual Ruination links/guards: $($s | ConvertTo-Json -Compress)"}
}
$phases=@($rows | Where-Object {$_ -like "phase`t*"} | ForEach-Object {($_ -split "`t",2)[1] | ConvertFrom-Json})
foreach($e in @(9,10)){
 $objective=if($e -eq 9){110556}else{110606}
 if(@($phases | Where-Object {$_.entry -eq $e -and $_.objective -eq $objective -and $_.negative -eq 1}).Count -ne 2){throw 'Phase boundary missing or duplicated'}
}
'PASS: migration twice in temporary tables; artifact loot link, five script bindings, native phase boundaries and previous scenario preserved'
