$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-split-geometry"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 $s=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
 $a=$s.IndexOf('void WorldObject::MovePositionToFirstCollision(');$b=$s.IndexOf('void WorldObject::MovePositionToTransportCollision(',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native first-collision body missing'}
 [IO.File]::WriteAllText("$out/FirstCollision.inc",$s.Substring($a,$b-$a))
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarSplitGeometry.cpp -ProbeName StromkarSplitGeometry -OutputFile "$out/native.tsv"
 Get-Content "$out/native.tsv" -Tail 1
}
