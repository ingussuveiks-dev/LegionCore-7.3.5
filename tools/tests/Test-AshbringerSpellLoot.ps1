param([switch]$VerifyReleaseBinary,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-spell-loot"
New-Item -ItemType Directory -Force $out|Out-Null
$name='2026_10_26_02_ashbringer_spell_loot.sql'
$migration=Get-Content "$repo/sql/updates/world/$name" -Raw
$scope='SourceTypeOrReferenceId=12 AND SourceGroup=199827'
$tables=@('gameobject_template','gameobject','spell_loot_template','conditions','spell_script_names')
$sql=@'
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN(247318,249824);
CREATE TEMPORARY TABLE saved_gameobject LIKE gameobject;
INSERT INTO saved_gameobject SELECT * FROM gameobject WHERE id IN(247318,249824);
CREATE TEMPORARY TABLE saved_spell_loot_template LIKE spell_loot_template;
INSERT INTO saved_spell_loot_template SELECT * FROM spell_loot_template WHERE Entry IN(199827,4954);
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=12 AND SourceGroup IN(199827,4954);
CREATE TEMPORARY TABLE saved_spell_script_names LIKE spell_script_names;
INSERT INTO saved_spell_script_names SELECT * FROM spell_script_names WHERE spell_id=199827;
'@
foreach($table in $tables){$sql+="`nCREATE TEMPORARY TABLE $table LIKE saved_$table;"}
$reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$reset+="`nDELETE FROM spell_loot_template WHERE Entry=199827;DELETE FROM conditions WHERE $scope;DELETE FROM spell_script_names WHERE spell_id=199827;"
$snapshot="SELECT '{label}-loot',current.* FROM spell_loot_template current ORDER BY Entry,Item;SELECT '{label}-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;"
$sql+="$reset`nSELECT 'baseline',COUNT(*) FROM spell_loot_template WHERE Entry=199827;`n$migration`n$($snapshot.Replace('{label}','first'))`n$migration`n$($snapshot.Replace('{label}','second'))"
foreach($table in @('gameobject_template','gameobject','spell_script_names')){$sql+="SELECT '$table-before',current.* FROM saved_$table current ORDER BY 2;SELECT '$table-after',current.* FROM $table current ORDER BY 2;`n"}
$sql+="SELECT 'unrelated-before',current.* FROM saved_spell_loot_template current WHERE Entry<>199827 ORDER BY Entry,Item;SELECT 'unrelated-after',current.* FROM spell_loot_template current WHERE Entry<>199827 ORDER BY Entry,Item;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'native',Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM spell_loot_template WHERE Entry=199827;"
$variants=[ordered]@{
 'script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=247318;"
 'ai'="UPDATE gameobject_template SET AIName='SmartGameObjectAI' WHERE entry=247318;"
 'link'='UPDATE gameobject_template SET Data1=247318 WHERE entry=247318;'
 'display'='UPDATE gameobject_template SET displayId=9806 WHERE entry=247318;'
 'flags'='UPDATE gameobject_template SET flags=0 WHERE entry=247318;'
 'personal'='UPDATE gameobject_template SET Data30=1 WHERE entry=247318;'
 'group'='UPDATE gameobject_template SET Data15=1 WHERE entry=247318;'
 'security'='UPDATE gameobject_template SET Data31=1 WHERE entry=247318;'
 'condition-id'='UPDATE gameobject_template SET Data17=1 WHERE entry=247318;'
 'phase'="UPDATE gameobject SET PhaseId='5940' WHERE id=247318;"
 'map'='UPDATE gameobject SET map=1220 WHERE id=247318;'
 'extra-spawn'='UPDATE gameobject SET id=247318 WHERE id=249824;'
 'existing-loot'='UPDATE spell_loot_template SET Entry=199827 WHERE Entry=4954;'
 'existing-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(12,199827,0,4,1,'custom');"
 'existing-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(199827,'custom');"
}
foreach($variant in $variants.Keys){$sql+="$reset`n$($variants[$variant])`n$($snapshot.Replace('{label}',"$variant-before"))`n$migration`n$($snapshot.Replace('{label}',"$variant-after"))"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ashbringer spell-loot SQL fixture failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
function Rows([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Rows $label))}
foreach($pair in @(@('first-loot','second-loot'),@('first-gates','second-gates'),@('unrelated-before','unrelated-after'))+@('gameobject_template','gameobject','spell_script_names'|ForEach-Object {,@("$_-before","$_-after")})){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Repeated/unrelated spell-loot content changed: $pair"}}
foreach($variant in $variants.Keys){foreach($suffix in @('loot','gates')){if((Signature "$variant-before-$suffix") -cne (Signature "$variant-after-$suffix")){throw "Custom definition not preserved: $variant"}}}
if((Rows 'baseline') -ne '0' -or (Rows 'native') -ne "199827`t120978`t0`t0`t100`t0`t1`t0`t1`t1" -or @(Rows 'gate').Count -ne 14){throw 'Missing exact native retrieval loot/rules'}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]](Rows 'gate'))
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -AshbringerRowsPath "$out/conditions.tsv"
if(!$? -or $LASTEXITCODE){throw 'Native spell-loot condition/recheck regression failed'}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/EffectConstants.inc",(Block (Get-Content "$repo/src/server/game/Spells/Spell.h" -Raw) 'enum SpellEffectHandleMode')+';'+(Block (Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw) 'enum SpellEffects')+';'+(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum InventorySlot')+';'+(Block (Get-Content "$repo/src/server/game/Entities/Item/Item.h" -Raw) 'enum InventoryResult')+';')
[IO.File]::WriteAllText("$out/CreateItem.inc",(Block $effects 'void Spell::EffectCreateItem2('))
[IO.File]::WriteAllText("$out/LootCrafting.inc",(Block (Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw) 'bool SpellInfo::IsLootCrafting('))
$store=Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'bool Player::AutoStoreLoot('
$first=$store.IndexOf('            ItemPosCountVec dest;');$last=$store.IndexOf('            SendNewItem(pItem,',$first);$last=$store.IndexOf(';',$last)+1
if($first -lt 0 -or $last -le $first){throw 'Native AutoStoreLoot item branch missing'}
[IO.File]::WriteAllText("$out/StoreItem.inc",$store.Substring($first,$last-$first))
$first=$store.IndexOf('    if (max_slot > 0)')
if($first -lt 0){throw 'Native AutoStoreLoot result changed'}
[IO.File]::WriteAllText("$out/StoreResult.inc",$store.Substring($first,$store.LastIndexOf('}')-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$sanitizer=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $sanitizer /I`"$out`" `"$PSScriptRoot/AshbringerSpellLootTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native retrieval/storage fragment regression failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerLootCatalog.cpp -ProbeName AshbringerLootCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native retrieval/hotfix catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("EFFECT`t180850`t2`t64`t0`t0`t199827`t0","EFFECT`t199827`t0`t157`t0`t59868`t0`t0","EFFECT`t181257`t0`t249`t0`t120978`t0`t120978","ITEM`t120978`t2`t17`tAshbringer","MAP`t1500`t1`t1")){if($native -notcontains $expected){throw "Native retrieval source changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/$name" -Algorithm SHA1).Hash
 $live=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'release',hash,state FROM updates WHERE name='$name';SELECT 'native',Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM spell_loot_template WHERE Entry=199827;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'chest',Data1 FROM gameobject_template WHERE entry=247318;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "native`t$(Rows 'native')" -or $live -notcontains "chest`t0"){throw 'Native spell-loot migration not deployed or chest changed'}
 if(($live|Where-Object {$_ -like "gate`t*"}|ForEach-Object {$_.Substring(5)}) -join "`n" -cne (Signature 'gate')){throw 'Live native loot rules differ'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_spell_loot_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed native retrieval consumer missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS native199827 source12 loot, exact fourteen rules and fifteen custom/repeat variants; native storage effects checked separately. No direct weapon grant/chest link or client/whole-chain certification.'
