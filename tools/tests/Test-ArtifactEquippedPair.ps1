param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-equipped-pair/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$name){$a=$source.IndexOf($name);if($a -lt 0){throw "Missing $name"};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($source[$b] -eq '{'))-([int]($source[$b] -eq '}'));++$b};$source.Substring($a,$b-$a)}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Paired native26972 metadata failed:$out/catalog.log"}
  $pairs=@(Get-Content "$out/native.tsv"|Where-Object {$_ -match '^CHILD\t(128908|128289)\t'}|ForEach-Object {$_.Substring(6)})
  if($pairs.Count -ne 2 -or "128908`t134553`t16" -notin $pairs -or "128289`t128288`t15" -notin $pairs){throw 'Matching native item pairs changed'}
  [IO.File]::WriteAllLines("$out/pairs.tsv",[string[]]@("128908`t134553`t16","128289`t128288`t15"),[Text.UTF8Encoding]::new($false))
  $effectPath='src/server/game/Spells/SpellEffects.cpp';$playerPath='src/server/game/Entities/Player/Player.cpp'
  if($Baseline){$effect=(& git -C $repo show "e8ab13ee1:$effectPath") -join "`n";$player=(& git -C $repo show "e8ab13ee1:$playerPath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-repair methods missing'}}else{$effect=Get-Content "$repo/$effectPath" -Raw;$player=Get-Content "$repo/$playerPath" -Raw}
  foreach($pair in @(@('ForceEquip',(Block $effect 'void Spell::EffectForceEquipItem(')),@('PairPreflight',(Block $player 'InventoryResult Player::CanEquipChildItem(')),@('PairEquip',(Block $player 'void Player::EquipChildItem(')))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",$pair[1])}
  $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  $flag=if($Baseline){'/DBASELINE'}else{''}
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $flag /I`"$out`" `"$PSScriptRoot/ArtifactEquippedPairTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/pairs.tsv`"`nexit /b %errorlevel%`n")
  & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 12;throw 'Actual paired-equipment method regression failed'}
  Get-Content "$out/test.log"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
