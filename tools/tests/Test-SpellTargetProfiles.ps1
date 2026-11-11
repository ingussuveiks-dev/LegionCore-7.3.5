$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe SpellTargetProfilesNative
if(!$? -or $LASTEXITCODE){throw 'Native difficulty-specific target hooks failed'}
