param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-the-three")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$root="$PSScriptRoot/../.."
$source=Get-Content "$root/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$source.IndexOf('bool Spell::SpellDummyTriggered(');$b=$source.IndexOf('void Spell::EffectDummy(',$a)
$function=$source.Substring($a,$b-$a)
$start=$function.IndexOf('            switch (itr.option)')
$caseStart=$function.IndexOf('                case DUMMY_TRIGGER_CAST_OR_REMOVE:')
$caseEnd=$function.IndexOf('                case DUMMY_TRIGGER_DAM_MAXHEALTH:',$caseStart)
$tail=$function.IndexOf('            if(itr.group != 0 && check)',$caseEnd)
if($a -lt 0 -or $b -lt 0 -or $start -lt 0 -or $caseStart -lt 0 -or $tail -lt 0){throw 'Missing production dummy trigger sections'}
$extracted=$function.Substring(0,$start)+"            switch (itr.option) {`n"+$function.Substring($caseStart,$caseEnd-$caseStart)+"            }`n"+$function.Substring($tail)
$case11=$function.IndexOf('                case DUMMY_TRIGGER_CAST_IGNORE_GCD:')
$case12=$function.IndexOf('                case DUMMY_TRIGGER_MOVE_AURA:',$case11)
if($case11 -lt 0 -or $case12 -lt 0){throw 'Missing production dummy option 11'}
$extracted=$extracted.Replace("            switch (itr.option) {`n","            switch (itr.option) {`n"+$function.Substring($case11,$case12-$case11))
[IO.File]::WriteAllText("$output/ThreeDummy.inc",$extracted)
$unit=Get-Content "$root/src/server/game/Entities/Unit/Unit.cpp" -Raw
$start=$unit.IndexOf('Unit* Unit::GetUnitForLinkedSpell(')
$unit=$unit.Substring($start,$unit.IndexOf('bool Unit::HasAuraLinkedSpell(',$start)-$start)
$cases=''
foreach($pair in @(@('LINK_UNIT_TYPE_CASTER','LINK_UNIT_TYPE_SELECTED'),@('LINK_UNIT_TYPE_TARGET','LINK_UNIT_TYPE_VICTIM'))){
 $a=$unit.IndexOf('        case '+$pair[0]+':');$b=$unit.IndexOf('        case '+$pair[1]+':',$a)
 if($a -lt 0 -or $b -lt 0){throw 'Missing production linked-unit selector'}
 $cases+=$unit.Substring($a,$b-$a)
}
[IO.File]::WriteAllText("$output/ThreeLinkedTargets.inc","Unit* Unit::GetUnitForLinkedSpell(Unit* caster,Unit* target,int type,Unit*) { switch(type) {`n$cases } return nullptr; }`n")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainTheThreeTest.cpp" /Fe"$output/HighmountainTheThreeTest.exe" /Fo"$output/HighmountainTheThreeTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainTheThreeTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'The Three production regression failed'}
