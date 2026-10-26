param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_11_warrior_creation_actions.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warrior-creation-actions"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarriorCreationActionCatalog.cpp -ProbeName WarriorCreationActionCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=@(Get-Content "$out/native.tsv")
 foreach($row in @("SPELL`t78`t0","SPELL`t88161`t0","SPELL`t6603`t1")){if($native -notcontains $row){throw 'Native creation spell catalog changed'}}
 $migration=Get-Content $MigrationPath -Raw
 $sql=@"
CREATE TEMPORARY TABLE saved_actions LIKE playercreateinfo_action;
INSERT INTO saved_actions SELECT * FROM playercreateinfo_action;
CREATE TEMPORARY TABLE playercreateinfo_action LIKE saved_actions;
INSERT INTO playercreateinfo_action SELECT * FROM saved_actions;
-- Pin the reported Orc defaults even after the live data has been repaired.
REPLACE INTO playercreateinfo_action(race,class,button,action,type) VALUES(2,1,1,78,0),(2,1,73,88161,0);
SELECT 'baseline-orc',COUNT(*) FROM playercreateinfo_action WHERE race=2 AND class=1 AND type=0 AND ((button=1 AND action=78) OR(button=73 AND action=88161));
-- Conflicting/custom positions and nonspell actions must survive.
REPLACE INTO playercreateinfo_action(race,class,button,action,type) VALUES(2,1,130,78,0),(2,1,131,88161,128);
$migration
$migration
SELECT 'bad-defaults',COUNT(*) FROM playercreateinfo_action WHERE class=1 AND type=0 AND ((button=1 AND action=78) OR(button=73 AND action=88161));
SELECT 'custom',COUNT(*) FROM playercreateinfo_action WHERE race=2 AND class=1 AND ((button=130 AND action=78 AND type=0) OR(button=131 AND action=88161 AND type=128));
SELECT 'removed-unrelated',COUNT(*) FROM saved_actions a LEFT JOIN playercreateinfo_action b ON a.race=b.race AND a.class=b.class AND a.button=b.button AND a.action=b.action AND a.type=b.type WHERE NOT(a.class=1 AND a.type=0 AND ((a.button=1 AND a.action=78) OR(a.button=73 AND a.action=88161))) AND b.race IS NULL;
SELECT 'orc-attack',COUNT(*) FROM playercreateinfo_action WHERE race=2 AND class=1 AND button=0 AND action=6603 AND type=0;
"@
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Creation defaults TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
 if(($rows -join '|') -ne "baseline-orc`t2|bad-defaults`t0|custom`t2|removed-unrelated`t0|orc-attack`t1"){throw "Creation default regression mismatch: $($rows -join '|')"}
 'PASS actual matching spell absence and TEMP counterfactual/migration twice; only obsolete Warrior defaults removed, real attack/custom positions/nonspell and all other races/classes preserved. No saved character row changed.'
}
