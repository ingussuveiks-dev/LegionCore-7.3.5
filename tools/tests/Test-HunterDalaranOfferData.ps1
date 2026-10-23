$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hunter-dalaran-offer"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=[IO.File]::ReadAllText("$repo/sql/updates/world/2026_10_23_00_hunter_dalaran_invitation.sql")
$sql=@"
CREATE TEMPORARY TABLE saved_spell_area LIKE spell_area;
INSERT INTO saved_spell_area SELECT * FROM spell_area;
CREATE TEMPORARY TABLE spell_area LIKE saved_spell_area;
INSERT INTO spell_area SELECT * FROM saved_spell_area;
UPDATE spell_area SET quest_end_status=74 WHERE spell=196908 AND area=7502 AND quest_end=40384 AND classmask=4;
$migration
$migration
SELECT 'offer',COUNT(*) FROM spell_area WHERE spell=196908 AND area=7502 AND quest_start=0 AND quest_end=40384 AND classmask=4 AND autocast=1 AND quest_end_status=1;
SELECT 'unrelated',COUNT(*) FROM spell_area a JOIN saved_spell_area b USING(spell,area,quest_start,quest_end) WHERE a.spell<>196908 AND a.quest_end_status<>b.quest_end_status;
UPDATE spell_area SET quest_end_status=33 WHERE spell=196908 AND area=7502 AND quest_end=40384;
$migration
SELECT 'custom',COUNT(*) FROM spell_area WHERE spell=196908 AND area=7502 AND quest_end=40384 AND quest_end_status=33;
SELECT 'questgiver',COUNT(*) FROM creature_queststarter WHERE id=100786 AND quest=40384;
SELECT 'giver-flag',COUNT(*) FROM creature_template WHERE entry=100786 AND (npcflag & 2)<>0 AND AIName='SmartAI';
SELECT 'personal-follow',COUNT(*) FROM smart_scripts WHERE entryorguid=100786 AND source_type=0 AND action_type IN (29,207) AND target_type=7;
SELECT 'accept-cleanup',COUNT(*) FROM smart_scripts WHERE entryorguid=100786 AND source_type=0 AND event_type=19 AND event_param1=40384 AND action_type=41;
"@
$query="$out/fixture.sql";[IO.File]::WriteAllText($query,$sql)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=$sql | & 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Hunter invitation SQL fixture failed'}
 $expected=@("offer`t1","unrelated`t0","custom`t1","questgiver`t1","giver-flag`t1","personal-follow`t2","accept-cleanup`t1")
 if(($rows -join '|') -ne ($expected -join '|')){throw "Wrong invitation fixture result: $($rows -join '|')"}
}finally{$env:MYSQL_PWD=$previous}
'PASS: repeatable Hunter invitation migration; unrelated/custom masks preserved; native personal/follow/questgiver/acceptance cleanup rows present (not client traversal).'
