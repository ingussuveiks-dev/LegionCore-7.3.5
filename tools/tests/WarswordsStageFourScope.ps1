# Older visibility fixtures isolate their original transition after this exact
# source certificate. The new suite compiles the current whole callback/instance.
function Remove-WarswordsStageFourScope([string]$source){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp'
 $old=(& git -C $repo show "6a5b1f80b:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned stage4 source'}
 function StageBlock([string]$s){$a=$s.IndexOf('void onScenarionNextStep(');if($a -lt 0){throw 'Missing stage callback'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed stage callback'};$s.Substring($a,$b-$a)}
 $actual=StageBlock $source;$prior=StageBlock $old
 if(($actual -replace '\s','') -ceq ($prior -replace '\s','')){return $source}
 $sha=[Security.Cryptography.SHA256]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($actual.Replace("`r",''))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}
 if($hash -ne '18098885e5aca5a270887bb0bed255727c44a806d758d0c0e51c28fb641099f4'){throw 'Uncertified later stage4 callback'}
 $normal=$source.Replace($actual,$prior).Replace('#include <memory>','').Replace('std::shared_ptr<uint8> sceneLifetime = std::make_shared<uint8>();','').Replace('sceneLifetime = std::make_shared<uint8>();','')
 if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other instance/arrival/wave/graveyard behavior changed'}
 $normal
}
