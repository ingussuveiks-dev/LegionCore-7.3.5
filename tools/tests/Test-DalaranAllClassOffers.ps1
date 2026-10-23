param([string]$ArtifactAudit="$PSScriptRoot/../../docs/audits/artifact-routes-2026-10-23.json")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dalaran-all-classes"
New-Item -ItemType Directory -Force $out | Out-Null
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DalaranClassOffersCatalog.cpp -ProbeName DalaranClassOffersCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
$pairs=@($native | Where-Object {$_ -like "PAIR`t*"} | ForEach-Object {$f=$_.Split("`t");"$($f[1])`t$($f[2])"})
if($pairs.Count -ne 149 -or @($native | Where-Object {$_ -like "SPEC`t*"}).Count -ne 36){throw 'Incomplete native playable catalog'}
[IO.File]::WriteAllLines("$out/pairs.tsv",[string[]]$pairs)
$spells=@($native | Where-Object {$_ -like "SPELL`t*"} | ForEach-Object {$_.Split("`t")[1]})
if($spells.Count -ne 22){throw 'Unexpected invitation catalog'}
$ids=$spells -join ','
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_03_dalaran_class_invitations.sql" -Raw
$sql=''
foreach($table in @('spell_area','quest_template_addon','smart_scripts')){
 $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table;CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$sql+=@"
UPDATE spell_area SET quest_end_status=74 WHERE spell IN ($ids);
UPDATE quest_template_addon SET AllowableClasses=0 WHERE ID IN (40714,41035);
UPDATE smart_scripts SET event_param1=40716 WHERE entryorguid=93775 AND source_type=0 AND id=2;
$migration
$migration
SELECT 'offers',COUNT(*) FROM spell_area WHERE spell IN ($ids) AND quest_end_status=1;
SELECT 'unrelated',COUNT(*) FROM spell_area a JOIN saved_spell_area b USING(spell,area,quest_start,aura_spell,racemask,classmask,active_event,gender) WHERE a.spell NOT IN ($ids) AND a.quest_end_status<>b.quest_end_status;
SELECT 'DK',AllowableClasses FROM quest_template_addon WHERE ID=40714;
SELECT 'Mage',AllowableClasses FROM quest_template_addon WHERE ID=41035;
SELECT 'Warrior',event_param1 FROM smart_scripts WHERE entryorguid=93775 AND source_type=0 AND id=2;
UPDATE spell_area SET quest_end_status=33 WHERE spell=216443 AND area=7502;
UPDATE quest_template_addon SET AllowableClasses=16 WHERE ID=40714;
UPDATE smart_scripts SET event_param1=999999 WHERE entryorguid=93775 AND source_type=0 AND id=2;
$migration
SELECT 'custom-offer',COUNT(*) FROM spell_area WHERE spell=216443 AND area=7502 AND quest_end_status=33;
SELECT 'custom-class',AllowableClasses FROM quest_template_addon WHERE ID=40714;
SELECT 'custom-SAI',event_param1 FROM smart_scripts WHERE entryorguid=93775 AND source_type=0 AND id=2;
"@
$config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Read-World([string]$query){
 $old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$config[3];$result=@($query | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'All-class SQL failed'};return $result}finally{$env:MYSQL_PWD=$old}
}
$rows=Read-World $sql
foreach($expected in @("offers`t22","unrelated`t0","DK`t32","Mage`t128","Warrior`t41052","custom-offer`t1","custom-class`t16","custom-SAI`t999999")){if($rows -notcontains $expected){throw "Missing all-class migration result $expected"}}
$rows=Read-World "SELECT spell,area,quest_start,quest_start_status,quest_end,quest_end_status,racemask,classmask,gender FROM spell_area WHERE spell IN ($ids) ORDER BY spell;"
if($rows.Count -ne 22){throw 'Missing live class invitation'}
[IO.File]::WriteAllLines("$out/offers.tsv",[string[]]$rows)
foreach($line in $rows){$f=$line.Split("`t");if($f[5] -ne '1' -or $f[6] -ne '0' -or $f[8] -ne '2'){throw 'Unexpected live status/race/gender gate'}}
& "$PSScriptRoot/Test-DalaranClassIntro.ps1" -OffersFile "$out/offers.tsv" -PairsFile "$out/pairs.tsv"
$audit=Get-Content $ArtifactAudit -Raw | ConvertFrom-Json
if($audit.routes.Count -ne 36 -or $audit.native_race_class_pairs.Count -ne 149 -or $audit.findings.Count){throw 'Fresh artifact route audit incomplete or has unreviewed admission findings'}
foreach($route in $audit.routes){if($native -notcontains "CHOSEN_EFFECT`t$($route.selection_spell)`t$($route.chosen)"){throw 'Native chosen-quest effect mismatch'}}
$nativePairs=@($pairs | Sort-Object);$auditPairs=@($audit.native_race_class_pairs | ForEach-Object {"$($_[0])`t$($_[1])"} | Sort-Object)
if(($nativePairs -join '|') -ne ($auditPairs -join '|')){throw 'Native C++ and structural artifact race/class catalogs disagree'}
& "$PSScriptRoot/Test-ArtifactOnboarding.ps1" -AuditFile $ArtifactAudit
$rows=Read-World @'
SELECT 'classes',COUNT(*) FROM quest_template_addon WHERE (ID=41052 AND AllowableClasses=1) OR (ID=38710 AND AllowableClasses=2) OR (ID=40384 AND AllowableClasses=4) OR (ID=40832 AND AllowableClasses=8) OR (ID=40705 AND AllowableClasses=16) OR (ID=40714 AND AllowableClasses=32) OR (ID=39746 AND AllowableClasses=64) OR (ID=41035 AND AllowableClasses=128) OR (ID=40716 AND AllowableClasses=256) OR (ID=12103 AND AllowableClasses=512) OR (ID=40643 AND AllowableClasses=1024) OR (ID=39047 AND AllowableClasses=2048);
SELECT 'no-extra-admission',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN (19,20) AND SourceEntry IN (41052,38710,40384,40832,40705,40714,39746,41035,40716,12103,40643,39047);
SELECT 'givers',COUNT(*) FROM creature_queststarter s JOIN creature_template t ON t.entry=s.id WHERE (t.npcflag&2)<>0 AND (t.unit_flags&33554432)=0 AND ((s.id=93775 AND s.quest=41052) OR (s.id=92909 AND s.quest=38710) OR (s.id=100786 AND s.quest=40384) OR (s.id=102018 AND s.quest=40832) OR (s.id=101344 AND s.quest=40705) OR (s.id=91731 AND s.quest=39746) OR (s.id=103506 AND s.quest=40716) OR (s.id=98519 AND s.quest=12103) OR (s.id=101061 AND s.quest=40643) OR (s.id=99343 AND s.quest=39047));
SELECT 'Thrall',COUNT(*) FROM creature WHERE id=91731 AND map=1220 AND phaseMask=1;
'@
foreach($expected in @("classes`t12","no-extra-admission`t0","givers`t10","Thrall`t1")){if($rows -notcontains $expected){throw "Live introduction coverage failed: $expected"}}
$summons=@($native | Where-Object {$_ -like "EFFECT`t*"} | ForEach-Object {$f=$_.Split("`t");if($f[3] -eq '28'){"SELECT $($f[1]) AS spell,$($f[5]) AS entry"}})
$routes=$summons -join ' UNION ALL '
$rows=Read-World "SELECT 'summoned-givers',COUNT(*) FROM spell_area a JOIN ($routes) n ON n.spell=a.spell JOIN creature_template t ON t.entry=n.entry JOIN creature_queststarter s ON s.id=t.entry AND s.quest=a.quest_end WHERE (t.npcflag&2)<>0 AND (t.unit_flags&33554432)=0 AND t.AIName='SmartAI';SELECT 'acceptance',COUNT(DISTINCT a.spell) FROM spell_area a JOIN ($routes) n ON n.spell=a.spell JOIN smart_scripts s ON s.entryorguid=n.entry AND s.source_type=0 AND s.event_type=19 AND s.event_param1=a.quest_end;"
foreach($expected in @("summoned-givers`t20","acceptance`t20")){if($rows -notcontains $expected){throw "Missing messenger interaction data $expected"}}
foreach($expected in @("EFFECT`t200023`t0`t150`t0`t40714`t0`t0","EFFECT`t195356`t0`t150`t0`t41035`t0`t0")){if($native -notcontains $expected){throw 'Native DK/Mage self-offer route missing'}}
'PASS: all12 class starts,22 invitation masks,149 native race/class pairs,36 native artifact selections and production onboarding. Actor gameplay/visibility/dialogue and full chains remain client checks; later actor-link audit findings are retained.'
