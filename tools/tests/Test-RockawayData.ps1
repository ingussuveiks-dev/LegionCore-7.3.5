$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/rockaway-data"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Rockaway replay SQL failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_03_rockaway_coast.sql" -Raw
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_18_05_helarjar_fragment_counts.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_18_06_rockaway_channel.sql" -Raw)
$channelMigration=Get-Content "$repo/sql/updates/world/2026_10_18_06_rockaway_channel.sql" -Raw
$channelScope=@(Query "CREATE TEMPORARY TABLE saved_channel_bindings LIKE spell_script_names;
INSERT INTO saved_channel_bindings SELECT * FROM spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_channel_bindings;
INSERT INTO spell_script_names SELECT * FROM saved_channel_bindings;
INSERT IGNORE INTO spell_script_names VALUES(231433,'rockaway_test_other_hook'),(231432,'rockaway_test_unrelated_hook');
CREATE TEMPORARY TABLE prior_channel_bindings LIKE spell_script_names;
INSERT INTO prior_channel_bindings SELECT * FROM spell_script_names;
$channelMigration
$channelMigration
SELECT 'channel-scope-missing',COUNT(*) FROM prior_channel_bindings a LEFT JOIN spell_script_names b ON a.spell_id=b.spell_id AND a.ScriptName=b.ScriptName WHERE b.spell_id IS NULL AND NOT(a.spell_id=231433 AND a.ScriptName='spell_rockaway_channel');
SELECT 'channel-scope-extra',COUNT(*) FROM spell_script_names a LEFT JOIN prior_channel_bindings b ON a.spell_id=b.spell_id AND a.ScriptName=b.ScriptName WHERE b.spell_id IS NULL AND NOT(a.spell_id=231433 AND a.ScriptName='spell_rockaway_channel');
SELECT 'channel-scope-one',COUNT(*) FROM spell_script_names WHERE spell_id=231433 AND ScriptName='spell_rockaway_channel';")
foreach($pair in @('channel-scope-missing,0','channel-scope-extra,0','channel-scope-one,1')){if($channelScope -notcontains $pair.Replace(',',"`t")){throw "Channel binding migration changed unrelated hooks: $pair"}}
$channelScope | Set-Content "$out/channel-scope.tsv"
$shadow=''
foreach($table in @('creature_template','creature_template_movement','creature_template_scaling','creature','spell_script_names','spell_loot_template','creature_loot_template','conditions')){
 $shadow+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$select=@'
SELECT 'anchor',COUNT(*) FROM creature WHERE id=115133 AND map=1220;
SELECT 'scripts',COUNT(*) FROM creature_template WHERE (entry=115133 AND ScriptName='npc_rockaway_vision') OR (entry=115132 AND ScriptName='npc_rockaway_ritualist') OR (entry=116187 AND ScriptName='npc_rockaway_tentacle') OR (entry=116195 AND ScriptName='npc_rockaway_slime' AND (unit_flags & 33554432)=0);
SELECT 'movement',COUNT(*) FROM creature_template_movement WHERE CreatureId=115133 AND Ground=0 AND Flight=1 AND Rooted=1;
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(116187,116195) AND LevelScalingMin=110 AND LevelScalingMax=110;
SELECT 'aura',COUNT(*) FROM spell_script_names WHERE spell_id=231329 AND ScriptName='spell_rockaway_taint';
SELECT 'channel',COUNT(*) FROM spell_script_names WHERE spell_id=231433 AND ScriptName='spell_rockaway_channel';
SELECT 'condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=12 AND SourceGroup=229113 AND SourceEntry=142487 AND ConditionTypeOrReference=60 AND ConditionValue1=44746;
SELECT 'helya-range',MinCount,MaxCount,QuestRequired FROM creature_loot_template WHERE Entry=114537 AND Item=142487 AND Currency=0;
SELECT Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,44746 FROM spell_loot_template WHERE Entry=229113 AND Item=142487 AND Currency=0;
'@
$rows=@(Query "$shadow`n$migration`nSELECT 'FIRST'; $select`n$migration`nSELECT 'SECOND'; $select`nSELECT 'END';
SELECT 'other-spawns',COUNT(*) FROM saved_creature a LEFT JOIN creature b ON a.guid=b.guid WHERE a.id<>115133 AND b.guid IS NULL;
SELECT 'other-spell-loot',COUNT(*) FROM saved_spell_loot_template a LEFT JOIN spell_loot_template b ON a.Entry=b.Entry AND a.Item=b.Item AND a.Currency=b.Currency AND a.MinCount=b.MinCount AND a.MaxCount=b.MaxCount AND a.Chance=b.Chance AND a.QuestRequired=b.QuestRequired AND a.LootMode=b.LootMode AND a.GroupId=b.GroupId WHERE NOT(a.Entry=229113 AND a.Item=142487 AND a.Currency=0) AND b.Entry IS NULL;
SELECT 'other-creature-counts',COUNT(*) FROM saved_creature_loot_template a LEFT JOIN creature_loot_template b ON a.Entry=b.Entry AND a.Item=b.Item AND a.Currency=b.Currency AND a.MinCount=b.MinCount AND a.MaxCount=b.MaxCount AND a.Chance=b.Chance AND a.QuestRequired=b.QuestRequired AND a.LootMode=b.LootMode AND a.GroupId=b.GroupId WHERE NOT(a.Entry=114537 AND a.Item=142487 AND a.Currency=0) AND b.Entry IS NULL;
SELECT 'helya-settings',COUNT(*) FROM saved_creature_loot_template a JOIN creature_loot_template b ON a.Entry=b.Entry AND a.Item=b.Item AND a.Currency=b.Currency AND a.Chance=b.Chance AND a.QuestRequired=b.QuestRequired AND a.LootMode=b.LootMode AND a.GroupId=b.GroupId AND a.Reference=b.Reference WHERE a.Entry=114537 AND a.Item=142487 AND a.Currency=0;")
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$e=[Array]::IndexOf($rows,'END')
if($a -lt 0 -or $b -le $a -or $e -le $b -or ($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($e-1)] -join "`n")){throw 'Rockaway migration not idempotent'}
foreach($pair in @('anchor,1','scripts,4','movement,1','scaling,2','aura,1','channel,1','condition,1','other-spawns,0','other-spell-loot,0','helya-range,100,150,1','other-creature-counts,0','helya-settings,1')){if($rows -notcontains $pair.Replace(',',"`t")){throw "Rockaway data mismatch: $pair"}}
$rows | Set-Content "$out/replay.tsv"
$loot=@($rows | Where-Object {$_ -like "229113`t142487`t*"} | Select-Object -First 1)
if($loot.Count -ne 1){throw 'Missing actual Rockaway spell loot row'}
$loot | Set-Content "$out/loot.tsv"
$rockawayFields=$loot[0].Split("`t")
$helyaFields=($rows | Where-Object {$_ -like "helya-range`t*"} | Select-Object -First 1).Split("`t")
@("229113`t142487`t$($rockawayFields[6])`t$($rockawayFields[7])`t$($rockawayFields[3])", "114537`t142487`t$($helyaFields[1])`t$($helyaFields[2])`t$($helyaFields[3])") | Set-Content "$out/counts.tsv"
& "$PSScriptRoot/Test-LostArmy.ps1" *> "$out/lost-army.log"
Get-ChildItem "$repo/build-extractors/runtime-test/lost-army/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$PSScriptRoot/LostArmyLootTest.cpp" -Raw
$end=$fixture.IndexOf('int main(')
if($end -lt 0){throw 'Missing Lost Army fixture boundary'}
[IO.File]::WriteAllText("$out/RockawayLootFixture.inc",$fixture.Substring(0,$end))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/RockawayLootTest.cpp`" /Fe`"$out/RockawayLoot.exe`" /Fo`"$out/RockawayLoot.obj`"`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/RockawayLoot.exe`" `"$out/loot.tsv`"`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Rockaway native loot regression failed'}
$native=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
$a=$native.IndexOf('LootItem::LootItem(LootStoreItem const&');$b=$native.IndexOf('void LootItem::InitItem(', $a)
if($a -lt 0 -or $b -le $a){throw 'Missing native randomized loot count constructor'}
[IO.File]::WriteAllText("$out/NativeFragmentLootItem.inc",$native.Substring($a,$b-$a))
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/HelarjarFragmentCountTest.cpp`" /Fe`"$out/FragmentCounts.exe`" /Fo`"$out/FragmentCounts.obj`"`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/FragmentCounts.exe`" `"$out/counts.tsv`"`n"
[IO.File]::WriteAllText("$out/counts.cmd",$cmd)
& "$out/counts.cmd"
if($LASTEXITCODE){throw 'Native fragment count construction failed'}
'PASS: repeat migration, actual loot and scoped conditions; full inventory behavior is covered separately as a fixture/native AutoStoreLoot inspection.'
