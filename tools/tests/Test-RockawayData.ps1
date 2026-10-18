$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/rockaway-data"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Rockaway replay SQL failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_03_rockaway_coast.sql" -Raw
$shadow=''
foreach($table in @('creature_template','creature_template_movement','creature_template_scaling','creature','spell_script_names','spell_loot_template','conditions')){
 $shadow+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$select=@'
SELECT 'anchor',COUNT(*) FROM creature WHERE id=115133 AND map=1220;
SELECT 'scripts',COUNT(*) FROM creature_template WHERE (entry=115133 AND ScriptName='npc_rockaway_vision') OR (entry=115132 AND ScriptName='npc_rockaway_ritualist') OR (entry=116187 AND ScriptName='npc_rockaway_tentacle') OR (entry=116195 AND ScriptName='npc_rockaway_slime' AND (unit_flags & 33554432)=0);
SELECT 'movement',COUNT(*) FROM creature_template_movement WHERE CreatureId=115133 AND Ground=0 AND Flight=1 AND Rooted=1;
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(116187,116195) AND LevelScalingMin=110 AND LevelScalingMax=110;
SELECT 'aura',COUNT(*) FROM spell_script_names WHERE spell_id=231329 AND ScriptName='spell_rockaway_taint';
SELECT 'condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=12 AND SourceGroup=229113 AND SourceEntry=142487 AND ConditionTypeOrReference=60 AND ConditionValue1=44746;
SELECT Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,44746 FROM spell_loot_template WHERE Entry=229113 AND Item=142487 AND Currency=0;
'@
$rows=@(Query "$shadow`n$migration`nSELECT 'FIRST'; $select`n$migration`nSELECT 'SECOND'; $select`nSELECT 'END';
SELECT 'other-spawns',COUNT(*) FROM saved_creature a LEFT JOIN creature b ON a.guid=b.guid WHERE a.id<>115133 AND b.guid IS NULL;
SELECT 'other-spell-loot',COUNT(*) FROM saved_spell_loot_template a LEFT JOIN spell_loot_template b ON a.Entry=b.Entry AND a.Item=b.Item AND a.Currency=b.Currency WHERE a.Entry<>229113 AND b.Entry IS NULL;")
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$e=[Array]::IndexOf($rows,'END')
if($a -lt 0 -or $b -le $a -or $e -le $b -or ($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($e-1)] -join "`n")){throw 'Rockaway migration not idempotent'}
foreach($pair in @('anchor,1','scripts,4','movement,1','scaling,2','aura,1','condition,1','other-spawns,0','other-spell-loot,0')){if($rows -notcontains $pair.Replace(',',"`t")){throw "Rockaway data mismatch: $pair"}}
$rows | Set-Content "$out/replay.tsv"
$loot=@($rows | Where-Object {$_ -like "229113`t142487`t*"} | Select-Object -First 1)
if($loot.Count -ne 1){throw 'Missing actual Rockaway spell loot row'}
$loot | Set-Content "$out/loot.tsv"
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
'PASS: repeat migration, actual loot and scoped conditions; full inventory behavior is covered separately as a fixture/native AutoStoreLoot inspection.'
