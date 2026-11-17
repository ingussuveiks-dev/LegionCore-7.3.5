# Exact inverse of the separately regressed, committed inventory-only amendment.
# Historical full-class certificates must still reject any foreign/partial edit.
function Restore-ArtifactInventorySource([string]$source,[string]$relativePath){
 $record=@(Get-Content "$PSScriptRoot/ArtifactInventorySourceRepairs.json" -Raw|ConvertFrom-Json|Where-Object path -eq $relativePath)
 if($record.Count -ne 1 -or $record[0].plans.Count -ne 1){throw 'Missing exact inventory amendment record'}
 $plan=$record[0].plans[0];$source=$source.Replace("`r",'')
 $hasher=[Security.Cryptography.SHA256]::Create()
 try{$hash=([BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($source)))).Replace('-','').ToLowerInvariant()}finally{$hasher.Dispose()}
 if($hash -eq $plan.before_sha256){return $source}
 if($hash -cne $plan.after_sha256){throw 'Unreviewed current source outside declared inventory amendment'}
 for($i=$plan.edits.Count-1;$i -ge 0;--$i){
  $edit=$plan.edits[$i]
  if([regex]::Matches($source,[regex]::Escape($edit.new)).Count -ne 1){throw 'Partial inventory source amendment'}
  $source=$source.Replace($edit.new,$edit.old)
 }
 $hasher=[Security.Cryptography.SHA256]::Create()
 try{$hash=([BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($source)))).Replace('-','').ToLowerInvariant()}finally{$hasher.Dispose()}
 if($hash -cne $plan.before_sha256){throw 'Inventory inverse differs from its pinned Git parent'}
 return $source
}
