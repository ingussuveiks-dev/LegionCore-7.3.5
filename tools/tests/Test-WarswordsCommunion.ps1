param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-WarswordsFinalAdmission.ps1" -CommunionBaseline
if($LASTEXITCODE){throw 'Pinned communion selection/damage baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -CommunionBaseline
if($LASTEXITCODE){throw 'Pinned communion victim-tick baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -ConversationBaseline
if($LASTEXITCODE){throw 'Pinned conversation failure baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -TimingBaseline
if($LASTEXITCODE){throw 'Pinned premature dialogue engagement baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -CleanupBaseline
if($LASTEXITCODE){throw 'Pinned overlapping dialogue baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -BlessingBaseline
if($LASTEXITCODE){throw 'Pinned early blessing implementation regression failed'}
& "$PSScriptRoot/Test-WarswordsFinalAdmission.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Repaired communion admission failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Repaired communion callback/retry failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-WarswordsConversationNative.ps1";if($LASTEXITCODE){throw 'Real conversation creation/removal/retry failed'}
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $out="$repo/build-extractors/runtime-test/warswords-final-intro/communion.tsv"
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCommunionCatalog.cpp -ProbeName WarswordsCommunionCatalog -OutputFile $out *> "$out.log";if(!$? -or $LASTEXITCODE){throw 'Native communion timing catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content $out
 foreach($expected in @("LINE`t2066`t100373`t0`t5000`t2067`t0`t1`t0`t60","LINE`t2067`t100662`t0`t1000`t0`t0`t0`t0`t60","SOUND`t100373`t60376`t1370031","SOUND`t100662`t60377`t1370032")){if($native -notcontains $expected){throw "Dialogue timing data changed: $expected"}}
 'PASS: matching native843 lines retain next-line link,5000/1000 padding and measured English voice file identities. Durations are separately measured local CASC evidence; no audio playback certification.'
}
