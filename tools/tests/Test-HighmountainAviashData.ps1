param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_07_highmountain_aviash.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('creature_template','spell_script_names','conditions') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'bird',COUNT(*) FROM creature_template WHERE entry=98778 AND AIName='' AND ScriptName='npc_highmountain_aviash' AND VehicleId=4428;
SELECT 'cast',COUNT(*) FROM spell_script_names WHERE spell_id=194359 AND ScriptName='spell_highmountain_aviash';
SELECT 'click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=98777 AND spell_id=194359 AND cast_flags=1;
SELECT 'source',COUNT(*) FROM creature WHERE guid=340585 AND id=98777 AND map=1220 AND ABS(position_x-5121.46)<0.02 AND ABS(position_y-4461.57)<0.02 AND ABS(position_z-1021.69)<0.02;
SELECT 'condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=98777 AND SourceEntry=194359 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=39387 AND NegativeCondition=0;
SELECT 'condition-total',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=98777 AND SourceEntry=194359;
SELECT 'no-old-ai',COUNT(*) FROM smart_scripts WHERE entryorguid=98778 AND source_type=0;
SELECT 'ritual',COUNT(*) FROM creature_template WHERE entry=98773 AND ScriptName='npc_skyhorn_ritual_guide';
"@
$expected=@("bird`t1","cast`t1","click`t1","source`t1","condition`t1","condition-total`t1","no-old-ai`t0","ritual`t1")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>98778 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName OR a.VehicleId<>b.VehicleId);
SELECT 'other-spells',COUNT(*) FROM saved_spell_script_names a LEFT JOIN spell_script_names b USING(spell_id,ScriptName) WHERE NOT(a.spell_id=194359 AND a.ScriptName='spell_highmountain_aviash') AND b.spell_id IS NULL;
SELECT 'other-conditions',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b USING(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition) WHERE NOT(a.SourceTypeOrReferenceId=18 AND a.SourceGroup=98777 AND a.SourceEntry=194359) AND b.SourceGroup IS NULL;
"@
 $expected+=@("other-ai`t0","other-spells`t0","other-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql
 if($LASTEXITCODE){throw 'Aviash SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Aviash SQL mismatch: $($actual -join '|')"}
'PASS: Aviash native spellclick/vehicle, quest condition, AI/spell bindings, twice-applied migration and unrelated-row preservation.'
