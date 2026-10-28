param([string]$MigrationPath,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/stromkar-central-field"
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_10_28_02_stromkar_central_field.sql"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $migration=Get-Content $MigrationPath -Raw;$sha=(Get-FileHash $MigrationPath -Algorithm SHA1).Hash
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 function Query([string]$sql){$prior=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Central field TEMP query failed'};$rows}finally{$env:MYSQL_PWD=$prior}}
 $sql=''
 foreach($table in @('creature','creature_template','creature_template_addon','creature_addon','smart_scripts','conditions')){
  $where=switch($table){'creature'{'id=101461'} 'creature_template'{'entry=101461'} 'creature_template_addon'{'entry=101461'} 'creature_addon'{'guid IN(SELECT guid FROM creature WHERE id=101461)'} 'smart_scripts'{'entryorguid=101461'} 'conditions'{'SourceTypeOrReferenceId=22 AND SourceEntry=101461'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $where;CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 if($VerifyLiveData){$sql+="DELETE FROM creature_addon WHERE guid=369687 AND path_id=0 AND mount=0 AND bytes1=0 AND bytes2=0 AND emote=0 AND auras='' AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_28_02_stromkar_central_field.sql' AND hash='$sha' AND state='RELEASED');`n"}
 $sql+="$migration`nSELECT 'first',creature_addon.* FROM creature_addon ORDER BY guid;`n$migration`nSELECT 'second',creature_addon.* FROM creature_addon ORDER BY guid;SELECT 'fixed',guid,path_id,mount,bytes1,bytes2,emote,CONCAT('[',auras,']') FROM creature_addon WHERE guid=369687;`n"
 foreach($table in @('creature','creature_template','creature_template_addon','smart_scripts','conditions')){$sql+="SELECT '$table-before',saved_$table.* FROM saved_$table;SELECT '$table-after',$table.* FROM $table;`n"}
 $sql+="SELECT 'other-before',saved_creature_addon.* FROM saved_creature_addon WHERE guid<>369687 ORDER BY guid;SELECT 'other-after',creature_addon.* FROM creature_addon WHERE guid<>369687 ORDER BY guid;`n"
 $variants=[ordered]@{
  'custom-addon'="INSERT INTO creature_addon(guid,auras) VALUES(369687,'34429');"
  'custom-template'="UPDATE creature_template_addon SET auras='34429' WHERE entry=101461;"
  'custom-pose'='UPDATE creature SET position_x=1 WHERE guid=369687;'
  'wrong-phase'="UPDATE creature SET PhaseId='5737' WHERE guid=369687;"
  'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101461;"
  'wrong-stage'='UPDATE conditions SET ConditionValue2=4 WHERE SourceGroup=2;'
  'wrong-reveal'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=9 AND id=0;'
  'wrong-boss'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=9 AND id=1;'
  'wrong-delay'='UPDATE smart_scripts SET event_param1=1 WHERE source_type=9 AND id=1;'
 }
 foreach($v in $variants.Keys){foreach($table in @('creature','creature_template','creature_template_addon','creature_addon','smart_scripts','conditions')){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"};$sql+="DELETE FROM creature_addon WHERE guid=369687;`n$($variants[$v])`nSELECT '$v-before',creature_addon.* FROM creature_addon ORDER BY guid;`n$migration`nSELECT '$v-after',creature_addon.* FROM creature_addon ORDER BY guid;`n"}
 $rows=Query $sql;[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 function Snapshot([string]$name){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$name`t*"}|ForEach-Object {$_.Substring($name.Length+1)}))}
 foreach($pair in @(@('first','second'),@('other-before','other-after'))){if((Snapshot $pair[0]) -cne (Snapshot $pair[1])){throw "Changed $pair"}}
 foreach($name in @('creature','creature_template','creature_template_addon','smart_scripts','conditions')+@($variants.Keys)){if((Snapshot "$name-before") -cne (Snapshot "$name-after")){throw "Unexpected change $name"}}
 if($rows -notcontains "fixed`t369687`t0`t0`t0`t0`t0`t[]"){throw 'Exact empty spawn override missing'}
 $source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw;$a=$source.IndexOf('CreatureAddon const* Creature::GetCreatureAddon() const');$b=$source.IndexOf('//creature_addon table',$a);if($a -lt 0 -or $b -le $a){throw 'Native addon selector missing'}
 $cpp=@'
#include <cassert>
#include <cstdint>
struct CreatureAddon{bool aura=true;};struct CreatureTemplate{unsigned Entry=101461;};
struct Manager{CreatureAddon base,spawn{false};bool overridden=false;CreatureAddon const*GetCreatureAddon(unsigned id){return overridden&&id==369687?&spawn:nullptr;}CreatureAddon const*GetCreatureTemplateAddon(unsigned id){return id==101461?&base:nullptr;}}manager;auto*sObjectMgr=&manager;
struct Creature{unsigned m_DBTableGuid=369687;CreatureTemplate row;CreatureTemplate const*GetCreatureTemplate()const{return &row;}CreatureAddon const*GetCreatureAddon()const;};
'@
 $cpp+=$source.Substring($a,$b-$a)+@'
int main(){Creature c;assert(c.GetCreatureAddon()->aura);manager.overridden=true;assert(!c.GetCreatureAddon()->aura);c.m_DBTableGuid=369668;assert(c.GetCreatureAddon()->aura);c.m_DBTableGuid=0;assert(c.GetCreatureAddon()->aura);c.row.Entry=1;assert(!c.GetCreatureAddon());}
'@
 [IO.File]::WriteAllText("$out/Addon.cpp",$cpp)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/test.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /fsanitize=address /Zi Addon.cpp /Fe:Addon.exe`nif errorlevel 1 exit /b 1`nAddon.exe`nexit /b %errorlevel%`n")
 & "$out/test.cmd";if($LASTEXITCODE){throw 'Native addon precedence failed'}
 if($VerifyLiveData){$live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_28_02_stromkar_central_field.sql';SELECT 'fixed',guid,path_id,mount,bytes1,bytes2,emote,CONCAT('[',auras,']') FROM creature_addon WHERE guid=369687;";if($live -notcontains "release`t$sha`tRELEASED" -or $live -notcontains "fixed`t369687`t0`t0`t0`t0`t0`t[]"){throw 'Live central field migration/override differs'}}
 'PASS actual-schema TEMP migration twice, nine custom refusals, unchanged shared actors/SmartAI/conditions, ASAN native addon precedence. Native reveal/rendering and full Soth sequence remain client checks.'
}
