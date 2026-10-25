param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify Release'}
& "$PSScriptRoot/Test-TruthguardPickupAdmission.ps1" -EquipBaseline:$Baseline -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Truthguard paired delivery host failed'}
if($VerifyReleaseBinary){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
 $first=$source.IndexOf('void GameObject::Update(')
 if($source.IndexOf('m_Functions.Update(diff);',$first) -ge $source.IndexOf('switch (m_lootState)',$first)){throw 'Deferred retry no longer precedes native deactivated cleanup'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_equipped_pair_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Truthguard paired delivery failed'}}finally{$env:PYTHONPATH=$previous}
}
