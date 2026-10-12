param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_17_highmountain_battle_worms.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('quest_objectives','spell_dummy_trigger') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'journey-objectives',COUNT(*) FROM quest_objectives WHERE QuestID=38915;
SELECT 'worm-objectives',COUNT(*) FROM quest_objectives WHERE QuestID=39776 AND Type=0 AND Amount=1 AND Flags=1 AND Flags2=0 AND ((ID=279939 AND ObjectID=97013 AND StorageIndex=0) OR (ID=280504 AND ObjectID=99217 AND StorageIndex=1) OR (ID=280505 AND ObjectID=99216 AND StorageIndex=2));
SELECT 'objective-total',COUNT(*) FROM quest_objectives WHERE QuestID=39776;
SELECT 'hook',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=197252 AND spell_trigger=197253 AND ``option``=11 AND effectmask=1 AND handlemask=8 AND caster=5 AND target=3 AND aura=0 AND bp0=0 AND bp1=0 AND bp2=0;
SELECT 'hook-total',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=197252;
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(97013,99216,99217) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'worms',COUNT(*) FROM creature WHERE guid IN(284551,284553,284573) AND id IN(97013,99216,99217) AND map=1220 AND phaseId='';
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE (id=108434 AND quest=38915) OR (id=93846 AND quest=39776);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=93846 AND quest IN(38915,39776);
SELECT 'field-mayla',COUNT(*) FROM creature WHERE guid=11351491 AND id=93846 AND map=1220 AND phaseId='';
SELECT 'thunder-totem-mayla',COUNT(*) FROM creature WHERE guid=11348959 AND id=108434 AND map=1220 AND phaseId='2246';
SELECT 'mayla-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7731 AND SourceEntry=3 AND ConditionTypeOrReference=8 AND ConditionValue1=39580 AND NegativeCondition=0;
SELECT 'reward-next',COUNT(*) FROM quest_template WHERE (ID=38915 AND RewardNextQuest=39776) OR (ID=39776 AND RewardNextQuest=42088);
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(38915,39776);
"@
$expected=@("journey-objectives`t0","worm-objectives`t3","objective-total`t3","hook`t1","hook-total`t1","scaling`t3","worms`t3","starter`t2","enders`t2","field-mayla`t1","thunder-totem-mayla`t1","mayla-phase`t1","reward-next`t2","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-objectives',COUNT(*) FROM saved_quest_objectives a LEFT JOIN quest_objectives b USING(ID) WHERE a.ID<>280011 AND (b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2);
SELECT 'other-dummy',COUNT(*) FROM saved_spell_dummy_trigger a LEFT JOIN spell_dummy_trigger b ON a.spell_id=b.spell_id AND a.spell_trigger=b.spell_trigger AND a.option=b.option AND a.aura=b.aura WHERE a.spell_id<>197252 AND (b.spell_id IS NULL OR a.effectmask<>b.effectmask OR a.handlemask<>b.handlemask OR a.target<>b.target OR a.caster<>b.caster);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,Description) VALUES (280011,38915,0,0,93846,1,0,0,'Custom replacement');
"@
 $sql+="`n$migration`n"
 $sql+="SELECT 'custom-objective-preserved',COUNT(*) FROM quest_objectives WHERE ID=280011 AND Amount=1 AND Description='Custom replacement';"
 $expected+=@("other-objectives`t0","other-dummy`t0","custom-objective-preserved`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Battle Worms SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Battle Worms SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: deployed Snowblind journey, three kills, Hooked Tongue direction, scaling, Mayla phases/relations and reward links.'}
else{'PASS: repeated migration, native objective parity, reversed jump caster/target, unrelated-row preservation and guarded custom-objective preservation.'}
