param([switch]$BaselineReader)
$ErrorActionPreference='Stop'
$out="$PSScriptRoot/../../build-extractors/runtime-test/artifact-export-publication"
if($BaselineReader){$out+='-baseline-reader'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $source=Get-Content "$PSScriptRoot/../research/Export-ArtifactAudit.ps1" -Raw
 if($BaselineReader){$source=(& git -C "$PSScriptRoot/../.." show f629d7ed8:tools/research/Export-ArtifactAudit.ps1) -join "`n";if($LASTEXITCODE){throw 'Cannot read pinned pre-reader-fix source'}}
 $a=$source.IndexOf('function Publish-AuditRows(');$b=$source.IndexOf('function Export-Table(',$a)
 if($a -lt 0 -or $b -le $a){throw 'Missing actual publisher'}
 . ([scriptblock]::Create($source.Substring($a,$b-$a)))
 $prefix='{"id":1}'+"`r`n"+'{"id":2}'+"`r`n"
 $lockedCases=@()
 foreach($case in @('complete','missing-tail','count-mismatch','after-tail','partial-json')){
  $text=switch($case){'complete'{$prefix+"__ARTIFACT_EXPORT_ROWS__2`r`n"};'missing-tail'{$prefix};'count-mismatch'{$prefix+"__ARTIFACT_EXPORT_ROWS__3`r`n"};'after-tail'{$prefix+"__ARTIFACT_EXPORT_ROWS__2`r`n"+'{"id":3}'+"`r`n"};'partial-json'{"{`r`n__ARTIFACT_EXPORT_ROWS__1`r`n"}}
  $inputPath=[IO.Path]::GetFullPath("$out/$case.stdout");$destination=[IO.Path]::GetFullPath("$out/$case.jsonl")
  [IO.File]::WriteAllText($inputPath,$text,[Text.UTF8Encoding]::new($false));$hash=(Get-FileHash $inputPath).Hash
  if($case -eq 'complete'){$count=Publish-AuditRows $inputPath $destination -Json;if($count -ne 2 -or [IO.File]::ReadAllText($destination) -cne $prefix){throw 'Complete native prefix changed'}}
  else{$refused=$false;try{[void](Publish-AuditRows $inputPath $destination -Json)}catch{if($_.Exception.Message -notmatch 'row-count|Incomplete audit JSON'){throw};$refused=$true};if(!$refused -or (Test-Path $destination)){throw "Incomplete export published: $case"}}
  if((Get-FileHash $inputPath).Hash -ne $hash){throw 'Original native stdout was changed'}
  $locked=$false
  try{$exclusive=[IO.File]::Open($inputPath,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$exclusive.Dispose()}catch{if($_.Exception.InnerException -isnot [IO.IOException]){throw};$locked=$true}
  if($locked){$lockedCases+=$case;if(!$BaselineReader -or $case -notin @('after-tail','partial-json')){throw "Rejected native input reader remains open: $case"}}
 }
 if($BaselineReader){if(!$lockedCases.Count){throw 'Expected pinned reader-lock counterfactual did not reproduce'};"PASS expected pre-fix counterfactual: native input remains locked after rejected $($lockedCases -join ','); no successful ordinary publication claimed"}
 else{'PASS actual publisher immediately releases every accepted/rejected input for exclusive reuse; original stdout bytes remain unchanged'}
 'PASS actual exporter publisher retains native bytes, accepts complete count2, rejects valid-boundary truncation/missing trailer/post-trailer bytes/malformed JSON before publishing a file'
}
