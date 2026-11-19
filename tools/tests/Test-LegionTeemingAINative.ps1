param([Parameter(Mandatory=$true)][string]$ModelDataDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$modelPath=(Resolve-Path -LiteralPath $ModelDataDirectory).Path.Replace('\','/')
$out="$repo/build-extractors/runtime-test/legion-teeming-ai/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$catalog=Get-Content "$repo/docs/research/legion-teeming-reconstruction-2026-11-19.json" -Raw|ConvertFrom-Json
$leaders=($catalog.formations.leaderGUID|Sort-Object -Unique)-join ','
$sql=@"
CREATE TEMPORARY TABLE formation_seed_groups AS SELECT * FROM creature_formations WHERE leaderGUID IN($leaders);
CREATE TEMPORARY TABLE formation_seed_guids (guid BIGINT UNSIGNED PRIMARY KEY);
INSERT IGNORE INTO formation_seed_guids SELECT memberGUID FROM formation_seed_groups;
INSERT IGNORE INTO formation_seed_guids SELECT leaderGUID FROM formation_seed_groups;
INSERT IGNORE INTO formation_seed_guids SELECT guid FROM creature WHERE guid BETWEEN 900902000 AND 900902214;
CREATE TEMPORARY TABLE formation_seed_creature AS SELECT c.* FROM formation_seed_guids s JOIN creature c ON c.guid=s.guid;
CREATE TEMPORARY TABLE formation_seed_addon AS SELECT a.* FROM creature_addon a JOIN formation_seed_creature c ON a.guid=c.guid;
CREATE TEMPORARY TABLE formation_seed_paths (id INT UNSIGNED PRIMARY KEY);
INSERT IGNORE INTO formation_seed_paths SELECT path_id FROM formation_seed_addon WHERE path_id<>0;
INSERT IGNORE INTO formation_seed_paths SELECT a.path_id FROM creature_template_addon a JOIN formation_seed_creature c ON a.entry=c.id WHERE a.path_id<>0;
CREATE TEMPORARY TABLE formation_seed_waypoint AS SELECT w.* FROM formation_seed_paths s JOIN waypoint_data w ON w.id=s.id;
CREATE TEMPORARY TABLE creature LIKE formation_seed_creature;
INSERT INTO creature SELECT * FROM formation_seed_creature;
CREATE TEMPORARY TABLE creature_addon LIKE formation_seed_addon;
INSERT INTO creature_addon SELECT * FROM formation_seed_addon;
CREATE TEMPORARY TABLE waypoint_data LIKE formation_seed_waypoint;
INSERT INTO waypoint_data SELECT * FROM formation_seed_waypoint;
CREATE TEMPORARY TABLE creature_formations LIKE formation_seed_groups;
INSERT INTO creature_formations SELECT * FROM formation_seed_groups;
CREATE TEMPORARY TABLE ai_seed_entries (entry INT UNSIGNED PRIMARY KEY);
INSERT IGNORE INTO ai_seed_entries SELECT id FROM formation_seed_creature;
CREATE TEMPORARY TABLE ai_seed_scripts AS SELECT s.* FROM ai_seed_entries e JOIN smart_scripts s ON s.entryorguid=e.entry WHERE s.source_type=0;
INSERT INTO ai_seed_scripts SELECT s.* FROM formation_seed_guids g JOIN smart_scripts s ON s.entryorguid=-CAST(g.guid AS SIGNED) WHERE s.source_type=0;
CREATE TEMPORARY TABLE ai_seed_conditions AS SELECT c.* FROM conditions c JOIN ai_seed_entries e ON c.SourceEntry=e.entry WHERE c.SourceTypeOrReferenceId=22;
CREATE TEMPORARY TABLE smart_scripts LIKE ai_seed_scripts;
INSERT INTO smart_scripts SELECT * FROM ai_seed_scripts;
CREATE TEMPORARY TABLE conditions LIKE ai_seed_conditions;
INSERT INTO conditions SELECT * FROM ai_seed_conditions;
"@
[IO.File]::WriteAllText("$out/shadow.sql",$sql,[Text.UTF8Encoding]::new($false))
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$names=@('LEGION_DIALOGUE_CONFIG','LEGION_TEEMING_FORMATION_SQL','LEGION_MAW_MODEL_DATA','CL','_LINK_');$previous=@{}
foreach($name in $names){$previous[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
$inputs=@("$repo/tools/tests/LegionTeemingAINative.cpp","$repo/src/server/game/Entities/Creature/CreatureGroups.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp","$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp","$repo/docs/research/legion-teeming-reconstruction-2026-11-19.json","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$modelPath/mmaps/go31852.mmap","$modelPath/mmaps/go31853.mmap")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash);$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_TEEMING_FORMATION_SQL="$out/shadow.sql";$env:LEGION_MAW_MODEL_DATA=$modelPath
 $env:CL="$($previous['CL']) /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 $env:_LINK_="$($previous['_LINK_']) /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionTeemingAINative.cpp -ProbeName LegionTeemingAINative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Native formation verification failed:$out/compiler.log"}
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(AI|PASS)'}
  }
 }finally{Pop-Location}
}finally{
 foreach($name in $names){[Environment]::SetEnvironmentVariable($name,$previous[$name],'Process')}
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 foreach($item in $sealed){if((Get-FileHash -LiteralPath $item.Path).Hash -ne $item.Hash){throw 'Formation verification input changed'}}
}
"AI_OUTPUT $out"
