param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$dump=Get-Content "$repo/src/server/game/Tools/PlayerDump.cpp" -Raw -Encoding utf8
$low=$dump.Substring($dump.IndexOf('static bool FindTokNth'),$dump.IndexOf('std::string CreateDumpString')-$dump.IndexOf('static bool FindTokNth'))
$helpers=$low+(Block $dump 'void StoreGUID(QueryResult result, uint32 field,')+(Block $dump 'static bool ChangePetReference(')
[IO.File]::WriteAllText("$OutputDirectory/SixthDumpHelpers.inc",$helpers)
$quest=Get-Content "$repo/src/server/scripts/Spells/spell_quest.cpp" -Raw -Encoding utf8
$shaman=Get-Content "$repo/src/server/scripts/Spells/spell_shaman.cpp" -Raw -Encoding utf8
$betrug=Get-Content "$repo/src/server/scripts/Legion/VioletHoldLegion/boss_fel_lord_betrug.cpp" -Raw -Encoding utf8
$zone=Get-Content "$repo/src/server/scripts/Northrend/zone_zuldrak.cpp" -Raw -Encoding utf8
$a=$zone.LastIndexOf('enum ',$zone.IndexOf('SPELL_BATTLE_SHOUT'));$enum=Block $zone.Substring($a) 'enum '
$parts=@('namespace SArtifact {'+(Block $quest 'class spell_q_artifact_trait_unlock :')+';}','namespace SAncestral {'+(Block $shaman 'class spell_sha_ancestral_protection :')+';}','namespace SEvade {constexpr uint32 QUEST_AMPHITHEATER_ANGUISH_TUSKARRMAGEDDON=12935;'+$enum+';'+(Block $zone 'class npc_orinoko_tuskbreaker :')+';}','namespace SBetrug {'+(Block $betrug ' enum Spells')+';'+$betrug.Substring($betrug.IndexOf('Position const jumpPos[4]'),$betrug.IndexOf('//102446')-$betrug.IndexOf('Position const jumpPos[4]'))+(Block $betrug 'class spell_betrug_mighty_smash :')+';}')
$selected=[regex]::Replace(($parts -join "`n"),'(Prepare(?:Aura|Spell)Script\([^;]+\);)','$1'+"`npublic:")
[IO.File]::WriteAllText("$OutputDirectory/SixthScripts.inc",$selected)
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw -Encoding utf8;$a=$spell.IndexOf('            uint64 maxHPDeficit = 0;');$b=$spell.IndexOf("`n        }`n        else",$a);if($b -lt 0){$b=$spell.IndexOf("`r`n        }`r`n        else",$a)}
[IO.File]::WriteAllText("$OutputDirectory/SixthChain.inc",$spell.Substring($a,$b-$a))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw -Encoding utf8;$a=$player.IndexOf('void Player::ResurrectPlayer(')
[IO.File]::WriteAllText("$OutputDirectory/SixthRestore.inc",(Block $player.Substring($a) '    if (restore_percent > 0.0f)'))
