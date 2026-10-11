# Read-only live database checks. This audit requires no migration.
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$native=Get-Content "$repo/docs/audits/highmountain-pay-blood-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.credits.PSObject.Properties | ForEach-Object {"SELECT $($_.Name) Entry,$($_.Value[0]) KillCredit1,$($_.Value[1]) KillCredit2"}) -join ' UNION ALL '
$sql=@"
SELECT 'native-objective',COUNT(*) FROM quest_objectives WHERE ID=279704 AND QuestID=39588 AND Type=0 AND ObjectID=95290 AND Amount=10 AND StorageIndex=0 AND Flags=0 AND Flags2=0 AND Bugged=0;
SELECT 'only-objective',COUNT(*) FROM quest_objectives WHERE QuestID=39588;
SELECT 'native-credit-mismatch',COUNT(*) FROM ($rows) a LEFT JOIN creature_template_wdb b USING(Entry) WHERE b.Entry IS NULL OR a.KillCredit1<>b.KillCredit1 OR a.KillCredit2<>b.KillCredit2;
SELECT 'local-spawns',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE (w.Entry=95290 OR w.KillCredit1=95290 OR w.KillCredit2=95290) AND c.map=1220 AND c.areaId=7725 AND c.phaseMask=1 AND c.phaseId='' AND c.spawntimesecs>0 AND t.faction=2792 AND (t.unit_flags & 33554690)=0;
SELECT 'no-duplicate-smart-credit',COUNT(*) FROM smart_scripts WHERE action_type=33 AND action_param1=95290;
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=95421 AND quest=39588;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=95421 AND quest=39588;
SELECT 'refuge',COUNT(*) FROM creature WHERE guid=340571 AND id=95421 AND map=1220 AND phaseId='';
SELECT 'parallel-gate',COUNT(*) FROM quest_template_addon WHERE ID=39588 AND PrevQuestID=39381 AND NextQuestID=39426 AND ExclusiveGroup=-39391;
SELECT 'group-members',COUNT(*) FROM quest_template_addon WHERE ExclusiveGroup=-39391;
SELECT 'next-quest',COUNT(*) FROM quest_template_addon WHERE ID=39426 AND PrevQuestID=39391;
SELECT 'no-disable',COUNT(*) FROM disables WHERE sourceType=1 AND entry=39588;
SELECT 'no-extra-accept-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=39588;
"@
$expected=@("native-objective`t1","only-objective`t1","native-credit-mismatch`t0","local-spawns`t41","no-duplicate-smart-credit`t0","starter`t1","ender`t1","refuge`t1","parallel-gate`t1","group-members`t3","next-quest`t1","no-disable`t0","no-extra-accept-condition`t0")
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Pay With Blood SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Pay With Blood SQL mismatch: $($actual -join '|')"}
'PASS: native objective/credit mappings, 41 unphased hostile local spawns, no duplicate SmartAI credit, refuge turn-in and parallel prerequisite gate.'
