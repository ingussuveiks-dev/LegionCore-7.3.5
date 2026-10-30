#requires -Version 7.0
param([switch]$AddressSanitizer,[switch]$SkipSql,[switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$MigrationPath='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-ritual-torture/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Section($s,$a,$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $a"};$s.Substring($i,$j-$i)}
 function Block($s,$a){$i=$s.IndexOf($a);if($i -lt 0){throw "Missing native $a"};$j=$s.IndexOf('{',$i)+1;$depth=1;while($depth -and $j -lt $s.Length){if($s[$j] -eq '{'){++$depth};if($s[$j] -eq '}'){--$depth};++$j};if($depth){throw "Unclosed native block $a"};$s.Substring($i,$j-$i)}
 $shared=Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw
 $auras=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraDefines.h" -Raw
 $enums=(Block $auras 'enum AuraType')+';'
 foreach($enum in @('enum SpellAttr1','enum SpellAttr3','enum SpellAttr6')){$enums+=(Block $shared $enum)+';'}
 [IO.File]::WriteAllText("$out/RitualEnums.inc",$enums)
 $scriptHeader=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
 $hookTypes=''
 foreach($type in @('AuraEffectPeriodicFnType','AuraEffectUpdateFnType','AuraEffectApplicationModeFnType')){
  $match=[regex]::Match($scriptHeader,"typedef void\(CLASSNAME::\*$type\)\(([^;]+)\);")
  if(!$match.Success){throw "Missing native aura hook type $type"}
  $hookTypes+="template<class C> using Native$type = void(C::*)($($match.Groups[1].Value));`n"
 }
 [IO.File]::WriteAllText("$out/NativeHookTypes.inc",$hookTypes)
 $mgr=Get-Content "$repo/src/server/game/Spells/SpellMgr.cpp" -Raw
 $position=$mgr.IndexOf('204125  // Shadow Torture');if($position -lt 0){throw 'Missing per-caster Shadow Torture correction'}
 $start=$mgr.LastIndexOf('ApplySpellFix({',$position);$end=$mgr.IndexOf('});',$position)
 $callback=(Block $mgr.Substring($start,$end+1-$start) '[](SpellInfo* spellInfo)').Replace('[](SpellInfo* spellInfo)','void ApplyTortureFix(SpellInfo* spellInfo)')
 [IO.File]::WriteAllText("$out/TortureFix.inc",$callback)
 $native=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeAuraStack.inc",(Section $native 'bool Aura::CanStackWith(' 'bool Aura::IsProcOnCooldown('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/StromkarRitualTortureTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native continuous-torture regression failed'}
 if($VerifyNativeCatalog){
  $priorConfig=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarEarlyVisualCatalog.cpp -ProbeName StromkarEarlyVisualCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Native ritual visuals failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$priorConfig}
  $catalog=Get-Content "$out/native.tsv" -Raw
  foreach($expected in @("EFFECT`t204125`t0`t6`t4`t0`t0`t38`t0","TORTURE_RANGE`t4`t0`t0`t30`t30","VISUAL`t204125`t53383`t0`t0","EVENT`t53383`t7`t8`t62962","PROCEDURAL`t62962`t9141`t26`t4511`t0`t0`t1","TORTURE_CHAIN`t4511`t68`t27`t0`t255`t255`t255`t255`t0.5`t0.5","EFFECT`t201032`t0`t6`t23`t0`t201031`t1`t0")){if(!$catalog.Contains($expected)){throw "Native ritual appearance differs: $expected"}}
 }
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
  $priorPython=$env:PYTHONPATH
  try{
   $env:PYTHONPATH="$repo/.codex/pe-review-deps;$priorPython";$ErrorActionPreference='Continue'
   & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_ritual_torture_release.py" $ReleaseDirectory > "$out/pe.stdout.log" 2> "$out/pe.stderr.log"
   $code=$LASTEXITCODE;$ErrorActionPreference='Stop';Get-Content "$out/pe.stdout.log";Get-Content "$out/pe.stderr.log"
   if($code){throw 'Linked continuous-torture check failed'}
  }finally{$env:PYTHONPATH=$priorPython;$ErrorActionPreference='Stop'}
 }
 if($SkipSql){return}
 if(!$MigrationPath){$MigrationPath="$repo/sql/pending/world/2026_10_30_30_stromkar_continuous_torture.sql";if(!(Test-Path $MigrationPath)){$MigrationPath="$repo/sql/updates/world/2026_10_30_30_stromkar_continuous_torture.sql"}}
 $migration=Get-Content $MigrationPath -Raw
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/sql-reader" -Test {
  $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
  $script:tortureQuery=0
  function Query($sql,[switch]$Refusal){
   if(!$env:CODEX_SQL_READER_USER -or !$env:CODEX_SQL_READER_PASSWORD){throw 'Restricted SQL reader required'}
   $p="$out/query-$script:tortureQuery";++$script:tortureQuery;$prior=$env:MYSQL_PWD
   try{
    $env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
    $args=@("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names');if($Refusal){$args+='--force'}
    [IO.File]::WriteAllText("$p.sql",$sql)
    $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $args -InputFile "$p.sql" -OutputFile "$p.stdout" -ErrorFile "$p.stderr"
    [IO.File]::WriteAllText("$p.exit.txt",[string]$code);$errorText=[IO.File]::ReadAllText("$p.stderr")
    if($Refusal){if($errorText -notmatch 'Stromkar continuous-torture inputs differ' -or ([regex]::Matches($errorText,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected SQL refusal: $p"}}
    elseif($code -or $errorText){throw "Native TEMP/read query failed: $p"}
    return ,@([IO.File]::ReadAllLines("$p.stdout"))
   }finally{$env:MYSQL_PWD=$prior}
  }
  $rangeRows=Query "SELECT c.guid,SQRT(POW(c.position_x-t.position_x,2)+POW(c.position_y-t.position_y,2)+POW(c.position_z-t.position_z,2)) FROM creature c JOIN creature t ON t.guid=370922 WHERE c.guid IN(370936,370937,370943) ORDER BY c.guid;"
  [IO.File]::WriteAllLines("$out/receiver-range.tsv",[string[]]$rangeRows)
  if($rangeRows.Count -ne 3){throw 'Missing native ritualist placement'}
  foreach($row in $rangeRows){$distance=[double]::Parse(($row -split "`t")[1],[Globalization.CultureInfo]::InvariantCulture);if($distance -gt 30){throw 'Actual ritualist is outside the native Shadow Torture search range'}}
  $setup=''
  foreach($table in @('conditions','spell_script_names','creature','creature_template','smart_scripts')){
   $filter=switch($table){conditions{'SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(204125,215997,201031)'}spell_script_names{"spell_id IN(201031,201032,204125) OR ScriptName IN('spell_stromkar_ritual_channel','spell_stromkar_ritual_torture')"}creature{'guid IN(370922,370936,370937,370943,370944)'}creature_template{'entry IN(101875,103144,101461)'}smart_scripts{'entryorguid=101875'}}
   $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $filter;CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
  }
  $setup+="DELETE FROM spell_script_names WHERE spell_id IN(201032,204125);DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=204125;`n"
  $snapshot="SELECT 'TAG-bind',s.* FROM spell_script_names s ORDER BY spell_id,ScriptName;SELECT 'TAG-condition',c.* FROM conditions c ORDER BY SourceTypeOrReferenceId,SourceEntry,ElseGroup,ConditionTypeOrReference;"
  function Snapshot($rows,$prefix){[string]::Join("`n",@($rows|Where-Object{$_ -like "$prefix*"}|ForEach-Object{$_.Substring($prefix.Length)}))}
  $rows=Query ($setup+$migration+$snapshot.Replace('TAG','first')+$migration+$snapshot.Replace('TAG','second'))
  [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
  if((Snapshot $rows 'first') -cne (Snapshot $rows 'second')){throw 'Continuous-torture migration is not idempotent'}
  if(@($rows|Where-Object{$_ -like 'first-bind*'}).Count -ne 3 -or @($rows|Where-Object{$_ -like 'first-condition*13*204125*Thoradin'}).Count -ne 1){throw 'Expected native target and two new bindings missing'}
  $variants=@("INSERT INTO spell_script_names VALUES(201032,'custom');","INSERT INTO spell_script_names VALUES(201032,'spell_stromkar_ritual_channel');","DELETE FROM creature WHERE guid=370922;","UPDATE creature SET PhaseId='5737' WHERE guid=370936;","UPDATE creature_template SET ScriptName='custom' WHERE entry=101875;","UPDATE smart_scripts SET action_param1=1 WHERE id=6 AND source_type=0;","UPDATE smart_scripts SET action_param2=1 WHERE id=6 AND source_type=0;","DELETE FROM spell_script_names WHERE spell_id=201031;","INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,ConditionValue2)VALUES(13,1,204125,31,3,101461);")
  foreach($variant in $variants){$rows=Query ($setup+$variant+$snapshot.Replace('TAG','before')+$migration+$snapshot.Replace('TAG','after')) -Refusal;if((Snapshot $rows 'before') -cne (Snapshot $rows 'after')){throw 'Customized torture inputs were modified'}}
  if($VerifyLiveData){$name=[IO.Path]::GetFileName($MigrationPath);$sha=(Get-FileHash $MigrationPath -Algorithm SHA1).Hash.ToLower();$rows=Query "SELECT 'released',COUNT(*) FROM updates WHERE name='$name' AND hash='$sha' AND state='RELEASED';SELECT 'binding',spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(201032,204125);SELECT 'receiver',ConditionTypeOrReference,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=204125;";if($rows -notcontains "released`t1" -or $rows -notcontains "receiver`t31`t3`t103144" -or $rows -notcontains "binding`t201032`tspell_stromkar_ritual_channel" -or $rows -notcontains "binding`t204125`tspell_stromkar_ritual_torture"){throw 'Live continuous-torture migration differs'}}
  "PASS restricted native-schema TEMP replay/repeat and $($variants.Count) custom-input refusals; no persistent write"
 }
}
