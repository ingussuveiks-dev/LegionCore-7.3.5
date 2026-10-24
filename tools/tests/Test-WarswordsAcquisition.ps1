param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/warswords-acquisition-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$checks=@(
 @{Name='Test-DB2CommonDataRow.ps1';Parameters=@{}},
 @{Name='Test-WarswordsHelyaVisualData.ps1';Parameters=@{}},
 @{Name='Test-WarswordsCommunion.ps1';Parameters=@{}},
 @{Name='Test-WarswordsIntroBubble.ps1';Parameters=@{}},
 @{Name='Test-WarswordsHelyaChannel.ps1';Parameters=@{}},
 @{Name='Test-WarswordsSpectreRush.ps1';Parameters=@{FirstBoss=$true}},
 @{Name='Test-WarswordsSpectreRush.ps1';Parameters=@{}},
 @{Name='Test-WarswordsFinalVisibility.ps1';Parameters=@{}},
 @{Name='Test-WarswordsFirstReentry.ps1';Parameters=@{}},
 @{Name='Test-WarswordsWeaponPhase.ps1';Parameters=@{}},
 @{Name='Test-WarswordsRetreat.ps1';Parameters=@{}},
 @{Name='Test-WarswordsFinalCredit.ps1';Parameters=@{}},
 @{Name='Test-WarswordsPickupAdmission.ps1';Parameters=@{}},
 @{Name='Test-WarswordsPresentation.ps1';Parameters=@{}},
 @{Name='Test-WarswordsReturnCredit.ps1';Parameters=@{}},
 @{Name='Test-ScepterLogin.ps1';Parameters=@{}}
)
for($index=0;$index -lt $checks.Count;++$index){
 $check=$checks[$index]
 $parameters=$check.Parameters
 if($VerifyReleaseBinary -and $check.Name -ne 'Test-WarswordsHelyaVisualData.ps1'){$parameters.VerifyReleaseBinary=$true}
 $log="$out/$($index+1).log"
 $global:LASTEXITCODE=0
 try{& "$PSScriptRoot/$($check.Name)" @parameters *> $log;if(!$? -or $LASTEXITCODE){throw 'Regression returned failure'}}catch{Get-Content $log -Tail 20;throw "Failed $($check.Name): $_"}
 "PASS $($index+1)/$($checks.Count): $($check.Name) named=$($parameters.Keys -join ',') log=$log"
}
