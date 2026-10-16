param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-tier-progression"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$PSScriptRoot/../research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactPowerCatalog.cpp -ProbeName ArtifactPowerCatalog -OutputFile "$out/native.tsv"
if($LASTEXITCODE){throw 'Native artifact progression catalog failed'}
$source=Get-Content "$repo/src/server/game/Entities/Item/Item.cpp" -Raw
foreach($part in @(@('void Item::InitArtifactPowers(', 'void Item::ActivateFishArtifact(', 'Init.inc'),@('void Item::InitArtifactsTier(', 'void Item::CopyArtifactDataFromParent(', 'Tier.inc'),@('ItemDynamicFieldArtifactPowers const* Item::GetArtifactPower(', 'void Item::GiveArtifactXp(', 'Lookup.inc'),@('uint32 Item::GetTotalPurchasedArtifactPowers()', 'void Item::SetOwnerLevel(', 'Total.inc'))){
 $a=$source.IndexOf($part[0]);$b=$source.IndexOf($part[1],$a)
 if($a -lt 0 -or $b -le $a){throw "Production function missing: $($part[0])"}
 [IO.File]::WriteAllText("$out/$($part[2])",$source.Substring($a,$b-$a))
}
$header=Get-Content "$repo/src/server/game/Entities/Item/Item.h" -Raw
$a=$header.IndexOf('struct ItemDynamicFieldArtifactPowers');$b=$header.IndexOf('struct ItemDynamicFieldGems',$a)
[IO.File]::WriteAllText("$out/PowerStruct.inc",$header.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Handlers/ArtifactHandler.cpp" -Raw
$a=$source.IndexOf('void WorldSession::HandleArtifactAddPower(');$b=$source.IndexOf('void WorldSession::HandleArtifactAddRelicTalent(',$a)
[IO.File]::WriteAllText("$out/Purchase.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$source.IndexOf('    if (item->GetTotalPurchasedArtifactPowers() > 35')
$b=$source.IndexOf('    for (uint32 bonusListID :',$a)
if($a -lt 0 -or $b -le $a){throw 'Automatic artifact empowerment block missing'}
[IO.File]::WriteAllText("$out/AutoTier.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ArtifactTierProgressionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/native.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native artifact tier/purchase regression failed'}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$binary.Contains('Artifact tier: preserved prior power indexes for item %u')){throw 'Canonical Release lacks artifact tier index repair'}
 if(!$binary.Contains('Artifact empowerment: purchase threshold check for item %u')){throw 'Canonical Release lacks immediate artifact empowerment'}
}
$hashes=@(foreach($name in @('Artifact','ArtifactPower','ArtifactPowerRank','ArtifactPowerLink')){Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/$name.db2" -Algorithm SHA256 | Select-Object @{n='table';e={$name}},Hash})
$hashes | ConvertTo-Json | Set-Content "$out/native-hashes.json" -Encoding utf8
'PASS: extracted production initialization/indexing/purchases with native 7.3.5 powers, links and ranks. Continuous client purchase/reopen/relog verification pending.'
