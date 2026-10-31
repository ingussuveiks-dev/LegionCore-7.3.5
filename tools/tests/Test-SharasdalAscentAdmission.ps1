param([string]$DataFile='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$DataFile){$DataFile="$repo/build-extractors/runtime-test/sharasdal-ascent-admission-data/rows.tsv"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-ascent-admission"};$out=[IO.Path]::GetFullPath($OutputDirectory)
function Section([string]$s,[string]$a,[string]$b){$start=$s.IndexOf($a);$end=$s.IndexOf($b,$start);if($start -lt 0 -or $end -lt $start){throw "Missing $a"};$s.Substring($start,$end-$start)}
function Block([string]$s,[string]$a){$start=$s.IndexOf($a);if($start -lt 0){throw "Missing $a"};$end=$s.IndexOf('{',$start)+1;$depth=1;while($depth -and $end -lt $s.Length){$depth+=([int]($s[$end] -eq '{'))-([int]($s[$end] -eq '}'));++$end};if($depth){throw "Unclosed $a"};$s.Substring($start,$end-$start)}
& "$PSScriptRoot/Test-SharasdalReturnAdmission.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native condition/source18/dynamic-flag host preparation failed'}
$hostSource=(Get-Content "$out/NativeHost.inc" -Raw).Replace('identity=1066,step=6','identity=1066,step=3').Replace('return 105819;','return 38821;')
$hostSource=$hostSource.Replace('    SpellCastResult CastSpell(Unit*,uint32,bool);','    bool HasSpellCooldown(uint32)const{return false;} SpellCastResult CastSpell(Unit*,uint32,bool); SpellCastResult CastSpell(Unit* t,uint32 s,bool b,void*,void*,uint32 owner){assert(owner==1);return CastSpell(t,s,b);}')
$conditions=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw;$cases=''
$wrapper='bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows) { return IsObjectMeetToConditionList(info,rows); }'
if(!$hostSource.Contains($wrapper)){throw 'Reusable condition wrapper changed'}
$hostSource=$hostSource.Replace($wrapper,'bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&) const;')
$hostSource+="`n"+(Block $conditions 'bool ConditionMgr::IsObjectMeetToConditions(ConditionSourceInfo&')
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
foreach($pair in @(@('CLASS','RACE'),@('MAPID','AREAID'),@('ALIVE','HP_VAL'),@('SCENARION_STEP','REALM_ACHIEVEMENT'),@('QUESTTAKEN','QUEST_COMPLETE'),@('QUEST_COMPLETE','QUEST_NONE'),@('TYPE_MASK','RELATION_TO'))){$cases+=Section $conditions "        case CONDITION_$($pair[0]):" "        case CONDITION_$($pair[1]):"}
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/CastConditions.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
$prepare=Block $source 'SpellCastResult Spell::prepare('
if($prepare.IndexOf('SpellCastResult result = CheckCast(true);') -lt 0 -or $prepare.IndexOf('SpellCastResult result = CheckCast(true);') -gt $prepare.IndexOf('if ((_triggeredCastFlags & TRIGGERED_CAST_DIRECTLY)')){throw 'Triggered spell prepare no longer checks native cast admission before direct effects'}
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$click=Block $unit 'bool Unit::HandleSpellClick('
[IO.File]::WriteAllText("$out/Click.inc",$click.Substring(0,$click.IndexOf('        SpellInfo const* spellEntry'))+"        assert(caster==clicker&&target==clicker&&origCasterGUID==clicker->GetGUID());caster->CastSpell(target,itr->second.spellId,false);res=true;return res;`n    }`n    return res;`n}`n")
[IO.File]::WriteAllText("$out/RemoveModes.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum AuraRemoveMode')+';')
$aura=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
$linked=$aura.Substring($aura.IndexOf('if (std::vector<SpellLinked> const* spellTriggered = sSpellMgr->GetSpellLinked(SPELL_LINK_REMOVE'))
$linked=$linked.Substring($linked.IndexOf('                            if (!itr.removeMask'))
$guards=Section $linked '                            if (!itr.removeMask' '                            if(itr.chance'
[IO.File]::WriteAllText("$out/RemovalGuards.inc",$guards)
$linked=$linked.Substring($linked.IndexOf('                            switch (itr.actiontype)'))
[IO.File]::WriteAllText("$out/RemovalCast.inc",(Section $linked '                                case LINK_ACTION_DEFAULT:' '                                case LINK_ACTION_LEARN:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalAscentAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$DataFile`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ascent spell/click/linked removal admission failed'}
