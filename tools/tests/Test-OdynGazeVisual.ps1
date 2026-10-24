param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/odyn-gaze-visual"
New-Item -ItemType Directory -Force $out|Out-Null
$migrationPath="$repo/sql/updates/world/2026_10_24_55_odyn_gaze_visual_source.sql"
$migration=Get-Content $migrationPath -Raw
$hash=(Get-FileHash $migrationPath -Algorithm SHA1).Hash
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native gaze catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("GAZE_VISUAL_SPELL_COLLISION`t57404`tOut of Jotunheim!","GAZE_VISUAL_ROW`t111461`t57404`t230829`t0`t0`t0`t0`t0`t1`t0`t0","GAZE_EVENT`t122011`t57404`t9`t10`t69892","GAZE_KIT_EFFECT`t150914`t2`t326473","GAZE_ATTACHMENT`t326473`t27935`t255`t1`t0`t521019`t1`t0")){if($native -notcontains $expected){throw "Native gaze chain changed: $expected"}}
$visualRows=@($native|Where-Object {$_ -like "GAZE_VISUAL_ROW`t*"})
if($visualRows.Count -ne 1){throw 'Native portal visual selection is ambiguous'}
[IO.File]::WriteAllLines("$out/visual.tsv",[string[]]@($visualRows[0].Substring(16)))
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Gaze TEMP read failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$reset='DELETE FROM areatrigger_template;INSERT INTO areatrigger_template SELECT * FROM saved_templates;DELETE FROM areatrigger_actions;INSERT INTO areatrigger_actions SELECT * FROM saved_actions;'
$sql=@"
CREATE TEMPORARY TABLE saved_templates LIKE areatrigger_template;INSERT INTO saved_templates SELECT * FROM areatrigger_template WHERE customEntry=11869 OR entry=11869;
CREATE TEMPORARY TABLE saved_actions LIKE areatrigger_actions;INSERT INTO saved_actions SELECT * FROM areatrigger_actions WHERE customEntry=11869 OR entry=11869;
UPDATE saved_templates SET VisualID=57404 WHERE entry=0 AND spellId=0 AND customEntry=11869 AND VisualID=230829
AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_24_55_odyn_gaze_visual_source.sql' AND hash='$hash' AND state='RELEASED');
CREATE TEMPORARY TABLE areatrigger_template LIKE saved_templates;CREATE TEMPORARY TABLE areatrigger_actions LIKE saved_actions;
$reset
SELECT 'before',areatrigger_template.* FROM areatrigger_template ORDER BY entry,customEntry;
SELECT 'actions-before',areatrigger_actions.* FROM areatrigger_actions ORDER BY entry,customEntry,id;
$migration
SELECT 'first',areatrigger_template.* FROM areatrigger_template ORDER BY entry,customEntry;
SELECT 'visual',VisualID FROM areatrigger_template WHERE entry=0 AND spellId=0 AND customEntry=11869;
$migration
SELECT 'second',areatrigger_template.* FROM areatrigger_template ORDER BY entry,customEntry;
SELECT 'actions-after',areatrigger_actions.* FROM areatrigger_actions ORDER BY entry,customEntry,id;
"@
$variants=[ordered]@{
 'custom-visual'='UPDATE areatrigger_template SET VisualID=999 WHERE customEntry=11869;'
 'custom-entry'='UPDATE areatrigger_template SET entry=999 WHERE customEntry=11869;'
 'custom-spell'='UPDATE areatrigger_template SET spellId=999 WHERE customEntry=11869;'
 'custom-radius'='UPDATE areatrigger_template SET Radius=6 WHERE customEntry=11869;'
 'custom-height'='UPDATE areatrigger_template SET Height=1 WHERE customEntry=11869;'
 'custom-movement'='UPDATE areatrigger_template SET isMoving=1 WHERE customEntry=11869;'
 'custom-enter'='UPDATE areatrigger_actions SET spellId=999 WHERE customEntry=11869 AND id=0;'
 'custom-exit'='UPDATE areatrigger_actions SET moment=2 WHERE customEntry=11869 AND id=1;'
 'custom-target'='UPDATE areatrigger_actions SET targetFlags=2 WHERE customEntry=11869 AND id=0;'
 'missing-action'='DELETE FROM areatrigger_actions WHERE customEntry=11869 AND id=1;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',areatrigger_template.* FROM areatrigger_template ORDER BY entry,customEntry;`n$migration`nSELECT '$name-after',areatrigger_template.* FROM areatrigger_template ORDER BY entry,customEntry;`n"}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
function Snapshot([string]$label){[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]](Snapshot $label))}
if((Signature 'first') -cne (Signature 'second') -or (Signature 'actions-before') -cne (Signature 'actions-after')){throw 'Gaze replay changed actions or is not idempotent'}
foreach($name in $variants.Keys){if((Signature "$name-before") -cne (Signature "$name-after")){throw "Custom gaze overwritten: $name"}}
$before=Snapshot 'before';$after=Snapshot 'first'
if($before.Count -ne 2 -or $after.Count -ne 2){throw 'Expected separate custom gaze and noncustom flame keyspaces'}
$corrected=$before[0].Split("`t");$corrected[3]='230829'
if([string]::Join("`t",$corrected) -cne $after[0] -or $before[1] -cne $after[1] -or (Snapshot 'visual') -notcontains '230829'){throw 'Gaze repair changed geometry or noncustom Flame'}
[IO.File]::WriteAllLines("$out/selected.tsv",[string[]](Snapshot 'visual'))
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Spells/SpellMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/GetInfo.inc",(Block $source 'SpellInfo const* SpellMgr::GetSpellInfo('))
$first=$source.IndexOf('    for (SpellXSpellVisualEntry const* visual : sSpellXSpellVisualStore)',$source.IndexOf('void SpellMgr::LoadSpellInfoStore('));$last=$source.IndexOf('    for (SpellPowerEntry const* spellPower :',$first)
if($first -lt 0 -or $last -le $first){throw 'Native visual registration not found'}
[IO.File]::WriteAllText("$out/LoadVisuals.inc",$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw
[IO.File]::WriteAllText("$out/GetVisual.inc",(Block $source 'uint32 SpellInfo::GetSpellXSpellVisualId('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/UnitCondition.inc",(Block $source 'bool Unit::IsUnitMeetCondition('))
$source=Get-Content "$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp" -Raw
$first=$source.IndexOf('    if (info)',$source.IndexOf('    SetGuidValue(AREATRIGGER_FIELD_CREATING_EFFECT_GUID'))
$last=$source.IndexOf('    SetFloatValue(AREATRIGGER_FIELD_BOUNDS_RADIUS_2_D',$first)
if($first -lt 0 -or $last -le $first){throw 'Native area visual fields not found'}
[IO.File]::WriteAllText("$out/Resolve.inc",$source.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/OdynGazeVisualTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/visual.tsv`" `"$out/selected.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native gaze visual regression failed'}
if($VerifyReleaseBinary){
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_55_odyn_gaze_visual_source.sql';SELECT 'visual',VisualID FROM areatrigger_template WHERE entry=0 AND spellId=0 AND customEntry=11869;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "visual`t230829"){throw 'Live55 differs from tested visual source'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_odyn_gaze_visual_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical gaze visual resolver failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: scripted custom11869 resolves the actual native Warrior portal visual through230829, not unrelated57404 spell. TEMP/10 custom cases, native resolver and model/attachment chain checked; no actor spawn, placement, real aura/button/rendering or full departure proof.'
