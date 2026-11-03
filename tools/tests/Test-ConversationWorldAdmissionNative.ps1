param()
. "$PSScriptRoot/ConversationActorResolverPredecessor.ps1"
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/conversation-world-admission/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw;$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw 'Missing pinned factory'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed factory'};$s.Substring($a,$b-$a)}
$old=(& git -C $repo show '1bf86bb41:src/server/game/Entities/Conversation/Conversation.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned original factory missing'}
$body=(Block $old 'bool Conversation::CreateConversation(').Replace('bool Conversation::CreateConversation(','bool CreateConversation(').Replace('GetMap()->AddToMap(this)','GetMap()->AddToMap(static_cast<Conversation*>(this))')
[IO.File]::WriteAllText("$out/PinnedConversationAdmission.inc",'struct PinnedConversation : Conversation {'+$body+'};')
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   $path='src/server/game/Entities/Conversation/Conversation.cpp';$current=Get-PreConversationActorResolverSource (Get-Content "$repo/$path" -Raw)
   $factory=Block $current 'bool Conversation::CreateConversation('
   $a=$factory.IndexOf('    // AddToMap owns');$b=$factory.IndexOf('    return true;', $a)
   if($a -lt 0 -or $b -lt 0){throw 'Missing bounded world-admission repair'}
   $guarded=$factory.Remove($a,$b-$a)
   function Shape([string]$s){([regex]::Replace($s,'(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
   if((Shape $current.Replace($factory,$guarded)) -cne (Shape $old)){throw 'Only post-AddToMap caller-owned refusal cleanup may change; all dynamic payload/scopes/lifetime/DB loader retained'}
   'PASS entire Conversation predecessor retained except post-AddToMap actual world admission and caller-owned cleanup'
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/ConversationWorldAdmissionNative.cpp -ProbeName ConversationWorldAdmissionNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw "Native world-admission observation failed: $out/catalog.log"}
  }
 }finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed'}
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native DBErrors require investigation'}
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|OBSERVE|COUNTERFACTUAL)'}
