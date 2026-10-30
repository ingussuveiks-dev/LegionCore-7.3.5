param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-delayed-scene/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/sql-reader" -Test {
 function Section($s,$a,$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $a"};$s.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $s '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $s '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $s '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $s '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $s '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $s '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
 [IO.File]::WriteAllText("$out/SceneConditions.inc",$cases)
 [IO.File]::WriteAllText("$out/SceneElseGroups.inc",(Section $s 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject*'))
 $path="$repo/sql/updates/world/2026_10_30_29_stromkar_delayed_scene_stage.sql"
 $migration=Get-Content $path -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:sceneQuery=0
 function Query($sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$p="$out/query-$script:sceneQuery";++$script:sceneQuery
  try{
   if(!$env:CODEX_SQL_READER_USER -or !$env:CODEX_SQL_READER_PASSWORD){throw 'Restricted SQL reader required'}
   $env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
   $a=@("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names');if($Refusal){$a+='--force'}
   [IO.File]::WriteAllText("$p.sql",$sql)
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $a -InputFile "$p.sql" -OutputFile "$p.stdout" -ErrorFile "$p.stderr"
   [IO.File]::WriteAllText("$p.exit.txt",[string]$code);$err=[IO.File]::ReadAllText("$p.stderr")
   if($Refusal){if($err -notmatch 'Stromkar delayed-scene conditions differ' -or ([regex]::Matches($err,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $p"}}
   elseif($code -or $err){throw "Read/TEMP query failed: $p"}
   return ,@([IO.File]::ReadAllLines("$p.stdout"))
  }finally{$env:MYSQL_PWD=$old}
 }
 $setup=@'
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
'@
 $baseline=$setup+$migration+"DELETE FROM conditions WHERE ElseGroup>=2;`n"
 $snap="SELECT 'TAG',c.* FROM conditions c ORDER BY ElseGroup,ConditionTypeOrReference;"
 $rules="SELECT 'TAG',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions ORDER BY ElseGroup,ConditionTypeOrReference;"
 $rows=Query ($baseline+$rules.Replace('TAG','before-rule')+$migration+$snap.Replace('TAG','first')+$rules.Replace('TAG','after-rule')+$migration+$snap.Replace('TAG','second'))
 [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
 function Signature($r,$tag){[string]::Join("`n",@($r|Where-Object{$_ -like "$tag`t*"}|ForEach-Object{$_.Substring($tag.Length+1)}))}
 if((Signature $rows first) -cne (Signature $rows second)){throw 'Not idempotent'}
 foreach($tag in @('before-rule','after-rule')){[IO.File]::WriteAllText("$out/$tag.tsv",(Signature $rows $tag))}
 foreach($variant in @("UPDATE conditions SET ScriptName='custom';",'UPDATE conditions SET Comment=NULL;','UPDATE conditions SET ConditionValue1=2 WHERE ConditionTypeOrReference=15;','UPDATE conditions SET ConditionValue1=991 WHERE ConditionTypeOrReference=44;','DELETE FROM conditions WHERE ElseGroup=1;',"UPDATE conditions SET ConditionTarget=1 WHERE ConditionTypeOrReference=2;",'UPDATE conditions SET NegativeCondition=0 WHERE ConditionTypeOrReference=8;')){
  $r=Query ($baseline+$variant+$snap.Replace('TAG','before')+$migration+$snap.Replace('TAG','after')) -Refusal
  if((Signature $r before) -cne (Signature $r after)){throw 'Custom conditions changed'}
 }
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/StromkarDelayedSceneTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/before-rule.tsv`" `"$out/after-rule.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native delayed-scene gate regression failed'}
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 $native=[IO.File]::ReadAllText("$out/native.tsv")
 foreach($line in @("CRITERION`t9`t29552`t92`t49109`t0","CRITERION`t9`t29553`t92`t49110`t0","EFFECT`t206436`t2`t61`t0`t49110`t0`t1`t0")){if(!$native.Contains($line)){throw 'Native final criteria/event differs'}}
 $smart=Query "SELECT id,link,action_type,action_param1,action_param2 FROM smart_scripts WHERE entryorguid=247877 AND source_type=1 ORDER BY id;"
 if([string]::Join("`n",$smart) -cne "0`t1`t205`t92`t49110`n1`t2`t85`t206436`t0`n2`t0`t85`t206448`t0"){throw 'Native loot event/cast ordering differs'}
 if($VerifyLiveData){$sha=(Get-FileHash $path -Algorithm SHA1).Hash;$live=Query "SELECT hash,state FROM updates WHERE name='2026_10_30_29_stromkar_delayed_scene_stage.sql';SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436;";if($live.Count -ne 2 -or $live[0] -ne "$sha`tRELEASED" -or $live[1] -ne '28'){throw 'Live delayed-scene migration differs'}}
 'PASS guarded TEMP repeat/seven refusals, native final criteria and existing GO event-before-delayed-cast ordering. No live writes; actual client/equip remain separate checks.'
 }
}
