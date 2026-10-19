param([string]$AdminUser='root')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/trial-helya-corrosion-data";New-Item -ItemType Directory -Force $out | Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$connection=[regex]::Match($config,'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
function Query([string]$sql,[switch]$Admin){
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=if($Admin){$env:CODEX_MYSQL_ADMIN_PASSWORD}else{$connection[3]}
  $user=if($Admin){$AdminUser}else{$connection[2]}
  $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$user --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Helya corrosion data check failed'}
  $rows
 }finally{$env:MYSQL_PWD=$previous}
}
$rows=@(Query @'
SELECT 'duplicate',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=228062;
SELECT 'condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=228062 AND ConditionTypeOrReference=31 AND ConditionTarget=0 AND ConditionValue1=3 AND ConditionValue2=114537 AND NegativeCondition=0;
SELECT 'condition-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=228062;
SELECT 'area',COUNT(*) FROM areatrigger_template WHERE entry=8835 AND customEntry=12904 AND spellId=228121 AND Radius=8;
SELECT 'area-actions',COUNT(*) FROM areatrigger_actions WHERE entry=8835 AND spellId=228127 AND ((moment=1 AND actionType=0) OR (moment IN(2,8) AND actionType=1));
SELECT 'boss',COUNT(*) FROM creature_template WHERE entry=114537 AND ScriptName='boss_helya_tov';
SELECT 'orb',COUNT(*) FROM creature_template WHERE entry=114566 AND ScriptName='npc_orb_of_corrosion';
'@)
foreach($tag in @('condition','condition-count','area','boss','orb')){if($rows -notcontains "$tag`t1"){throw "Native corrosion data mismatch: $tag"}}
if($rows -notcontains "duplicate`t0" -or $rows -notcontains "area-actions`t3"){throw 'Duplicate corrosion route or native area cleanup data changed'}
$rows | Set-Content "$out/world.tsv"
# The migration runs only against a connection-local TEMPORARY table here.
# Production data receives no writes from this test. Exercise its exact SQL,
# repeat it and retain an unrelated spell and a customized same-key variant.
$migration=Get-Content "$repo/sql/updates/world/2026_10_19_00_helya_corrosion_decay.sql" -Raw
$fixture=@'
CREATE TEMPORARY TABLE codex_helya_dummy_shape LIKE spell_dummy_trigger;
CREATE TEMPORARY TABLE spell_dummy_trigger LIKE codex_helya_dummy_shape;
INSERT INTO spell_dummy_trigger (spell_id,spell_trigger,`option`,target,caster,targetaura,bp0,bp1,bp2,effectmask,handlemask,aura,`group`,comment) VALUES
(228062,0,11,0,0,0,0,0,0,1,8,0,0,'original'),
(228061,0,11,0,0,0,0,0,0,1,8,0,0,'unrelated');
'@
$test=$fixture+"`n"+$migration+"`nSELECT 'first',ROW_COUNT();`n"+$migration+"`nSELECT 'repeat',ROW_COUNT();`n"+@'
INSERT INTO spell_dummy_trigger (spell_id,spell_trigger,`option`,target,caster,targetaura,bp0,bp1,bp2,effectmask,handlemask,aura,`group`,comment)
VALUES(228062,0,11,0,1,0,0,0,0,1,8,0,0,'custom caster');
'@+"`n"+$migration+"`nSELECT 'custom',ROW_COUNT(); SELECT 'retained',COUNT(*) FROM spell_dummy_trigger;"
$result=@(Query $test -Admin)
foreach($expected in @("first`t1","repeat`t0","custom`t0","retained`t2")){if($result -notcontains $expected){throw "Scoped migration mismatch: $expected"}}
$result | Set-Content "$out/migration.tsv"
'PASS: native Helya condition/Decay actions retained; duplicate route absent; exact migration is repeatable and preserves unrelated/custom rows in a temporary table.'
