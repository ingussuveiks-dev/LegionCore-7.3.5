# Exact later wave delta; paired source/native regressions certify it separately.
function Read-SharasdalWaveBlock([string]$source){
 $a=$source.IndexOf('class npc_sharasdal_waterwall :');if($a -lt 0){throw 'Missing whole wave class'};$b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){$depth+=([int]($source[$b] -eq '{'))-([int]($source[$b] -eq '}'));++$b};if($depth){throw 'Unclosed wave class'};$source.Substring($a,$b-$a)
}
function Remove-SharasdalWaveScope([string]$source){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
 $old=(& git -C $repo show "dbb95ce8d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned original wave missing'}
 $original=Read-SharasdalWaveBlock $old;$actual=Read-SharasdalWaveBlock $source
 if($actual.Replace("`r",'') -ceq $original.Replace("`r",'')){return $source}
 $sha=[Security.Cryptography.SHA256]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($actual.Replace("`r",''))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}
 if($hash -ne '7d142fd348630b6576c3f6da94e62571e7fed8c6c345cfcc4a26384a88e12be8'){throw 'Uncertified later wave cleanup change'}
 $source.Replace($actual,$original)
}
