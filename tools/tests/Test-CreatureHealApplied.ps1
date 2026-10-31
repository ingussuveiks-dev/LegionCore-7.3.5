param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$PreviousRelease='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired regression can certify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$paths=@('src/server/game/Entities/Unit/Unit.cpp','src/server/game/Entities/Creature/Creature.cpp','src/server/game/Entities/Creature/Creature.h','src/server/game/AI/CreatureAI.h')
$sources=@{};$bases=@{}
foreach($path in $paths){$sources[$path]=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$bases[$path]=(& git -C $repo show "5a4a13649:$path") -join "`n";if($LASTEXITCODE){throw "Missing pinned $path"}}
$unit=$sources[$paths[0]];$creature=$sources[$paths[1]]
$old="    if (addhealth)`n        gain = victim->ModifyHealth(int32(addhealth), this);"
$new=@'
    if (addhealth)
    {
        gain = victim->ModifyHealth(int32(addhealth), this);
        if (Creature* creature = victim->ToCreature())
            creature->NotifyHealApplied(this, addhealth, gain);
    }
'@
$normalized=$unit.Replace($new,$old)
if($normalized.TrimEnd("`n") -cne $bases[$paths[0]].TrimEnd("`n")){throw 'Unit changed beyond post-ModifyHealth notification'}
$notify=Block $creature 'void Creature::NotifyHealApplied('
if($creature.Replace($notify+"`n`n",'').TrimEnd("`n") -cne $bases[$paths[1]].TrimEnd("`n")){throw 'Creature changed beyond notification'}
$declaration="        void NotifyHealApplied(Unit* healer, uint32 amount, int32 gain);`n`n"
if($sources[$paths[2]].Replace($declaration,'').TrimEnd("`n") -cne $bases[$paths[2]].TrimEnd("`n")){throw 'Creature header changed beyond nonvirtual declaration'}
$virtual=@'
public:
    // Final positive heal after HealDone and ModifyHealth. Gain can be zero
    // for overhealing; cancelled heals and negative health changes are excluded.
    // Append after existing virtuals so their slots remain unchanged.
    virtual void OnHealApplied(Unit* /*healer*/, uint32 /*amount*/, int32 /*gain*/) {}

'@
if($sources[$paths[3]].Replace($virtual+"`n",'').TrimEnd("`n") -cne $bases[$paths[3]].TrimEnd("`n")){throw 'CreatureAI changed beyond appended default hook'}
if($ScopeOnly){'PASS exact post-heal notification scope; existing virtual order, data members and all other core methods unchanged';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/creature-heal-applied"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$bases[$paths[0]]}else{$unit}
$heal=Block $selected 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal prefix boundary missing'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
[IO.File]::WriteAllText("$out/Notify.inc",$notify)
[IO.File]::WriteAllText("$out/Default.inc",(Block $sources[$paths[3]] 'virtual void OnHealApplied('))
[IO.File]::WriteAllText("$out/Destroy.inc",(Block $creature 'bool Creature::AIM_Destroy('))
$initialize=Block $creature 'bool Creature::AIM_Initialize('
[IO.File]::WriteAllText("$out/Initialize.inc",$initialize.Substring(0,$initialize.IndexOf('    AIM_Destroy();'))+"    (void)ai;`n    return true;`n}")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /O2 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/CreatureHealAppliedTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Creature final-heal regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $saved=$env:PYTHONPATH
 try{
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$saved"
  $peArguments=@("$PSScriptRoot/verify_creature_heal_applied_release.py","$repo/build-extractors/bin/Release")
  if($PreviousRelease){$peArguments+=@('--previous',$PreviousRelease)}
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe @peArguments
  if($LASTEXITCODE){throw 'Canonical final-heal notification missing'}
 }finally{$env:PYTHONPATH=$saved}
}
