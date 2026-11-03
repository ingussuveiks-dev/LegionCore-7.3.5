#requires -Version 7.0
param([switch]$BaselineSql,[switch]$VerifyLiveData,[string]$MigrationPath='',[string]$AudioDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($BaselineSql -and $VerifyLiveData){throw 'Counterfactual SQL cannot verify installed data'}
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_11_03_02_garona_outro_voice.sql"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/garona-outro-voice"}
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($out,"$out/audio","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-GaronaOutroVoiceAudio.ps1" -AudioDirectory $AudioDirectory -OutputDirectory "$out/audio"
 if(!$?){throw 'Native audio verification failed'}
 $fixture=Get-Content "$PSScriptRoot/GaronaOutroVoiceAudio.json" -Raw|ConvertFrom-Json
 $repair=Get-Content -LiteralPath $MigrationPath -Raw
 $migration=if($BaselineSql){''}else{$repair}
 $tables=[ordered]@{creature=@('entry','id','creatureId','creatureGuid','unk1','unk2','duration');actor=@('entry','id','actorId','creatureId','displayId','unk1','unk2','unk3','duration');data=@('entry','id','idx','textId','unk1','unk2')}
 function Payload([string]$tag,[string]$where='entry IN(2432)'){
  foreach($kind in $tables.Keys){$table="conversation_$kind";$label=if($kind -eq 'data'){'line'}else{$kind};"SELECT '$tag','$label',$($tables[$kind] -join ',') FROM $table WHERE $where ORDER BY entry,id;"}
 }
 $original=@()
 foreach($row in $fixture.OriginalCreatures){$original+="INSERT INTO saved_creature VALUES($($row -join ','));"}
 foreach($row in $fixture.OriginalActors){$original+="INSERT INTO saved_actor VALUES($($row -join ','));"}
 foreach($context in $fixture.Conversations){foreach($line in $context.Lines){$original+="INSERT INTO saved_data VALUES($($context.Conversation),$($line.Line),$($line.Index),$($line.OriginalStartMs),$($line.Flags),$($line.Packed));"}}
 $sql=@();$reset=@()
 foreach($kind in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$kind LIKE conversation_$kind; INSERT INTO saved_$kind SELECT * FROM conversation_$kind; DELETE FROM saved_$kind WHERE entry IN(2432);"}
 $sql+=$original
 foreach($kind in $tables.Keys){$sql+="CREATE TEMPORARY TABLE conversation_$kind LIKE saved_$kind;";$reset+="DELETE FROM conversation_$kind; INSERT INTO conversation_$kind SELECT * FROM saved_$kind;"}
 $sql+=$reset;$sql+=$migration;$sql+=@(Payload FIRST);$sql+=$migration;$sql+=@(Payload SECOND)
 # Compare every column and row count outside the two explicitly allowed cells.
 foreach($kind in $tables.Keys){
  $terms=@();foreach($column in $tables[$kind]){if($column -in @('entry','id')){continue};$term="NOT(c.$column <=> s.$column)"

   if($kind -eq 'creature' -and $column -eq 'duration'){$term="($term AND NOT(c.entry=2432 AND c.id=1))"}
   if($kind -eq 'data' -and $column -eq 'textId'){$term="($term AND NOT(c.entry=2432 AND c.id=5152))"}
   $terms+=$term
  }
  $sql+="SELECT 'preserved-$kind',COUNT(*) FROM conversation_$kind c JOIN saved_$kind s USING(entry,id) WHERE $($terms -join ' OR '); SELECT 'count-$kind',(SELECT COUNT(*) FROM conversation_$kind)=(SELECT COUNT(*) FROM saved_$kind);"
 }
 # Independently retry both canonical partial states for each two-line context.
 foreach($entry in @(2432)){foreach($half in @('duration','start')){
  $duration=11814;$start=10693
  $sql+=$reset;$sql+=if($half -eq 'duration'){"UPDATE conversation_creature SET duration=$duration WHERE entry=$entry AND id=1;"}else{"UPDATE conversation_data SET textId=$start WHERE entry=$entry AND idx=1;"}
  $sql+=$repair;$sql+=@(Payload "mixed-$entry-$half")
 }}
 $cases=@()
 foreach($entry in @(2432)){
  $kind='creature';$first=$fixture.Conversations|Where-Object Conversation -eq $entry;$line=$first.Lines[0].Line
  $variants=[ordered]@{
   duration="UPDATE conversation_$kind SET duration=1 WHERE entry=$entry AND id=0;"
   identity="UPDATE conversation_$kind SET creatureId=1 WHERE entry=$entry AND id=0;"
   guid="UPDATE conversation_$kind SET $('creatureGuid')=1 WHERE entry=$entry AND id=0;"
   actor_flags="UPDATE conversation_$kind SET unk1=1 WHERE entry=$entry AND id=0;"
   actor_flags2="UPDATE conversation_$kind SET unk2=1 WHERE entry=$entry AND id=0;"
   line_flags="UPDATE conversation_data SET unk1=1 WHERE entry=$entry AND id=$line;"
   packed="UPDATE conversation_data SET unk2=256 WHERE entry=$entry AND id=$line;"
   start="UPDATE conversation_data SET textId=1 WHERE entry=$entry AND id=$line;"
   index="UPDATE conversation_data SET idx=8 WHERE entry=$entry AND id=$line;"
   missing_actor="DELETE FROM conversation_$kind WHERE entry=$entry AND id=0;"
   missing_line="DELETE FROM conversation_data WHERE entry=$entry AND id=$line;"
   extra_actor="INSERT INTO conversation_$kind(entry,id) VALUES($entry,7);"
   extra_line="INSERT INTO conversation_data(entry,id,idx) VALUES($entry,999999,10);"
   opposite_kind="INSERT INTO conversation_$(if($kind -eq 'actor'){'creature'}else{'actor'})(entry,id) VALUES($entry,0);"
   line_identity="UPDATE conversation_data SET id=999999 WHERE entry=$entry AND id=$line;"
   other_field="UPDATE conversation_creature SET duration=1 WHERE entry=$entry AND id=1;"
  }
  foreach($name in $variants.Keys){$case="$entry-$name";$cases+=$case;$sql+=$reset;$sql+=$variants[$name];$sql+=@(Payload "before-$case" "entry=$entry");$sql+=$repair;$sql+=@(Payload "after-$case" "entry=$entry")}
 }
 $sql+="SELECT 'END-GARONA-VOICE',16;"
 [IO.File]::WriteAllText("$out/temp.sql",($sql -join "`n"))
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $priorAdmin=$env:CODEX_MYSQL_ADMIN_PASSWORD
 try{
  if($c[2] -eq 'root'){$env:CODEX_MYSQL_ADMIN_PASSWORD=$c[3]}
  & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/private-reader" -Test {
   $priorPassword=$env:MYSQL_PWD
   try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
    $arguments=@("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names')
    $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe' -Arguments $arguments -InputFile "$out/temp.sql" -OutputFile "$out/temp.tsv" -ErrorFile "$out/temp.stderr"
    if($code -or (Get-Item "$out/temp.stderr").Length){throw "TEMP-only SQL failed; inspect $out/temp.stderr"}
    if($VerifyLiveData){[IO.File]::WriteAllText("$out/live.sql",(@(Payload LIVE) -join "`n"));$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe' -Arguments $arguments -InputFile "$out/live.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr";if($code -or (Get-Item "$out/live.stderr").Length){throw 'Live read-only verification failed'}}
   }finally{$env:MYSQL_PWD=$priorPassword}
  }
 }finally{$env:CODEX_MYSQL_ADMIN_PASSWORD=$priorAdmin}
 $rows=Get-Content "$out/temp.tsv";if($rows[-1] -cne "END-GARONA-VOICE`t16"){throw 'TEMP output incomplete'}
 function Rows([string]$tag){@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}|Sort-Object)}
 function Same($expected,$actual,[string]$label){if(@($expected).Count -ne @($actual).Count -or (Compare-Object @($expected) @($actual))){throw "Native SQL mismatch: $label"}}
 $gold=@();foreach($row in $fixture.OriginalCreatures){$r=@($row);$r[6]=if($r[1] -eq 1){11814}else{2350};$gold+="creature`t$($r -join "`t")"}
 foreach($row in $fixture.OriginalActors){$r=@($row);throw 'Garona fixture must not contain model actors';$gold+="actor`t$($r -join "`t")"}
 foreach($context in $fixture.Conversations){foreach($line in $context.Lines){$gold+="line`t$($context.Conversation)`t$($line.Line)`t$($line.Index)`t$($line.StartMs)`t$($line.Flags)`t$($line.Packed)"}}
 $originalGold=@();foreach($row in $fixture.OriginalCreatures){$originalGold+="creature`t$($row -join "`t")"};foreach($row in $fixture.OriginalActors){$originalGold+="actor`t$($row -join "`t")"};foreach($context in $fixture.Conversations){foreach($line in $context.Lines){$originalGold+="line`t$($context.Conversation)`t$($line.Line)`t$($line.Index)`t$($line.OriginalStartMs)`t$($line.Flags)`t$($line.Packed)"}}
 Same $(if($BaselineSql){$originalGold}else{$gold}) @(Rows FIRST) 'first complete definitions';Same @(Rows FIRST) @(Rows SECOND) idempotence
 foreach($kind in $tables.Keys){foreach($expected in @("preserved-$kind`t0","count-$kind`t1")){if($rows -notcontains $expected){throw "Unrelated data changed: $kind"}}}
 foreach($entry in @(2432)){foreach($half in @('duration','start')){Same $gold @(Rows "mixed-$entry-$half") 'partial canonical retry'}}
 foreach($case in $cases){Same @(Rows "before-$case") @(Rows "after-$case") "custom $case"}
 if($VerifyLiveData){$live=@(Get-Content "$out/live.tsv"|ForEach-Object {$_.Substring(5)});Same $gold $live 'installed data'}
 [IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@(Rows FIRST))
 function Section([string]$source,[string]$start,[string]$end){$a=$source.IndexOf($start);$b=$source.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Native source section missing: $start"};$source.Substring($a,$b-$a)}
 $source=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
 [IO.File]::WriteAllText("$out/ConversationStructs.inc",(Section $source 'struct ConversationActor' 'typedef std::unordered_map'))
 $source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
 [IO.File]::WriteAllText("$out/ConversationData.inc",$source.Substring($source.IndexOf('bool Conversation::GenerateDynamicFieldData(')))
 [IO.File]::WriteAllText("$out/Lifetime.inc",(Section $source '    uint32 duration = 0;' '    if (!GetMap()->AddToMap(this))'))
 [IO.File]::WriteAllText("$out/Update.inc",(Section $source 'void Conversation::Update(uint32 p_time)' 'void Conversation::Remove()'))
 [IO.File]::WriteAllText("$out/Playback.inc",(Section $source 'bool Conversation::CanNeverSee2(' 'bool Conversation::LoadFromDB('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $mode=if($BaselineSql){'/DGARONA_VOICE_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/GaronaOutroVoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`" `"$out/audio/voices.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native ASAN producer regression failed'}
 [ordered]@{Counterfactual=[bool]$BaselineSql;LiveVerified=[bool]$VerifyLiveData;CustomPreservationCases=$cases.Count;MixedCanonicalRetries=2;MigrationSHA256=(Get-FileHash $MigrationPath).Hash;ClientPlaybackVerified=$false;CrossConversationCallersVerified=$false}|ConvertTo-Json|Set-Content "$out/summary.json"
 'PASS TEMP-only denied-persistent-write reader, two exact permitted cells, full unrelated columns/counts, twice idempotent, two partial retries,16 preserved custom definitions with complete unrelated-table preservation; actual native ASAN producer. Client/cross-conversation sequencing separate.'
}
