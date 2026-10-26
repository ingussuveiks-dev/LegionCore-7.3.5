param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-domination-retry"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "4717a90d:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned domination baseline unavailable'}
$current=Block $source 'class spell_bs_dominate_mind :'
$old=Block $pinned 'class spell_bs_dominate_mind :'
$first=$current.IndexOf('SpellCastResult result = caster->CastSpell(caster, 217042, true);')
$last=$current.IndexOf('else timer -= diff;',$first)
if($first -lt 0 -or $last -le $first){throw 'Reported-result branch missing'}
$tail=$current.Substring($first,$last-$first)
$restored=$current.Replace('bool willFailureReported = false;','').Replace("Unit* caster = GetCaster();`n                    if (!caster)",'if (!GetCaster())').Replace("Unit* caster = GetCaster();`r`n                    if (!caster)",'if (!GetCaster())').Replace('caster->GetInstanceScript()','GetCaster()->GetInstanceScript()').Replace($tail,"GetCaster()->CastSpell(GetCaster(), 217042, true);`n                            timer = 0;`n                        }`n                        ")
if(($restored -replace '\s','') -cne ($old -replace '\s','') -or ($source.Replace($current,$old) -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated domination/sequence behavior changed'}
if($Baseline){$current=$old}
$first=$current.IndexOf('bool unbrekable = false;');$last=$current.IndexOf('void Register()', $first)
if($first -lt 0 -or $last -le $first){throw 'Aura-owned state/hooks missing'}
[IO.File]::WriteAllText("$out/Aura.inc",$current.Substring($first,$last-$first).Replace('AuraEffectHandleModes','uint8'))
$header=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw
[IO.File]::WriteAllText("$out/Constants.inc",(Block $header 'enum Data')+';'+(Block $header 'enum eCreatures')+';'+(Block (Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw) 'enum SpellCastResult')+';')
$native=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
if($native -notmatch 'OnEffectUpdate' -or $current -notmatch 'OnEffectUpdate \+= AuraEffectUpdateFn\(spell_bs_dominate_mind_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_MOD_FACTION\)'){throw 'Native aura update binding changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=$(if($Baseline){'/DBASELINE '}else{''})+$(if($AddressSanitizer){'/fsanitize=address /Zi'}else{''})
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerDominationRetryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native domination retry regression failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerDominationCatalog.cpp -ProbeName AshbringerDominationCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native domination catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$rows=Get-Content "$out/native.tsv"
foreach($expected in @("OVERRIDE`t735`t216693","SPELL_MISSING`t216687","EFFECT`t216690`t0`t6`t243`t14`t0`t1`t0","EFFECT`t217042`t0`t6`t293`t735`t0`t18`t8","EFFECT`t216693`t1`t164`t0`t0`t216690`t1`t0","EFFECT`t216693`t3`t164`t0`t0`t217042`t1`t0")){if($expected -notin $rows){throw "Changed native domination reference: $expected"}}
Write-Host 'PASS native base+hotfix217042 override735/button216693/removal references; missing216687 explicitly retained as evidence, not fabricated'
if($VerifyReleaseBinary){$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_domination_retry_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed domination retry missing'}}finally{$env:PYTHONPATH=$previous}}
