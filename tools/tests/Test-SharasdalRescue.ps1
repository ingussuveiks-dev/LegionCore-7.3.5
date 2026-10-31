param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "dd6965c51:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned native rescue baseline missing'}
$old=Block $pinned 'class npc_adel_waterspeaker :';$current=Block $source 'class npc_adel_waterspeaker :'
foreach($method in @('npc_adel_waterspeakerAI(Creature*','CreatureAI* GetAI(','void EnterCombat(','void HealReceived(','void DamageTakenAfterScaling(','void MovementInform(','void UpdateAI(')){
 if((Block $current $method) -cne (Block $old $method)){throw "Unrelated Adelee method changed: $method"}
}
$normalized=$current
foreach($method in @('void AcceptRescue(','void CompleteRescue(','void NotifyGrash(')){$normalized=$normalized.Replace((Block $normalized $method),'')}
foreach($method in @('void Reset(','void UpdateLifecycle(')){$normalized=$normalized.Replace((Block $normalized $method),(Block $old $method))}
$fields=@'
        std::shared_ptr<uint8> _rescueLife = std::make_shared<uint8>();
        FunctionProcessor _rescueFunctions;
        bool _rescueAccepted = false, _rescueSchedule = false, _rescueDone = false;
        bool _rescueNotifyDone = false, _rescueNotifyQueued = false, _rescueNotifyExhausted = false;
        uint32 _rescueDelay = 0, _rescueNotifyTries = 0;
'@
$normalized=$normalized.Replace($fields,'')
# Restore the removed pre-cast click handler at its exact original class location.
$oldClick=Block $old 'void OnSpellClick('; $oldUpdate=Block $old 'void UpdateAI('
$normalized=$normalized.Replace($oldUpdate,$oldClick+"`n`n"+$oldUpdate)
if(($normalized -replace '\s','') -cne ($old -replace '\s','')){throw 'Unrelated Adelee state/combat/healing/path/factory changed'}
$whole=$source.Replace($current,$old).Replace((Block $source 'namespace SharasdalRescue'),'').Replace((Block $source 'class spell_sharasdal_breaking_bonds :')+';','')
foreach($header in @('Spell.h','SpellScript.h','Timer.h')){$whole=$whole.Replace("#include `"$header`"",'')}
$whole=$whole.Replace('    RegisterSpellScript(spell_sharasdal_breaking_bonds);','')
if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Other Sharasdal actor, Grash, instance, boss or pickup changed'}
if($ScopeOnly){'PASS exact Adelee rescue lifecycle/native cast guard scope; initial Grash/other actors/damage/healing/movement/pickup retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-rescue"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Policy.inc",(Block $source 'namespace SharasdalCompanion')+(Block $source 'namespace SharasdalGrash')+(Block $source 'namespace SharasdalRescue'))
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$old}else{$current})+';')
[IO.File]::WriteAllText("$out/Cast.inc",(Block $source 'class spell_sharasdal_breaking_bonds :')+';')
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($entry in @(@('Owner','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('ModifyHealth','int64 Unit::ModifyHealth('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $core $entry[1]))}
[IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
[IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary missing'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Notify.inc",(Block $creature 'void Creature::NotifyHealApplied('))
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
[IO.File]::WriteAllText("$out/TimerDiff.inc",(Block (Get-Content "$repo/src/common/Utilities/Timer.h" -Raw) 'inline uint32 getMSTimeDiff('))
[IO.File]::WriteAllText("$out/SendEvent.inc",(Block (Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw) 'void Spell::EffectSendEvent('))
function Adapt([string]$value,[string]$before,[string]$after){if(!$value.Contains($before)){throw "Native fixture boundary changed: $before"};$value.Replace($before,$after)}
$fixture=Get-Content "$PSScriptRoot/SharasdalGrashHealingTest.cpp" -Raw
$hostSource=$fixture.Substring(0,$fixture.IndexOf('#include "Policy.inc"'))
$hostSource=Adapt $hostSource 'enum{CLASS_SHAMAN=7,' 'enum{UNIT_FIELD_BYTES_1=1,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,CLASS_SHAMAN=7,'
$hostSource=Adapt $hostSource 'struct Unit;struct Player;struct Creature;' 'struct Unit;using WorldObject=Unit;struct Player;struct Creature;'
$hostSource=Adapt $hostSource 'struct Map{}liveMap,otherMap;' @'
inline uint32 nativeMapEvents=0,nativeInstanceEvents=0;
struct ZoneScript{void ProcessEvent(Unit*,uint32){assert(false);}};
struct Map{void ScriptsStart(uint32,uint32 event,Unit*,Unit*){assert(event==49816);++nativeMapEvents;}}liveMap,otherMap;
'@
$hostSource=Adapt $hostSource 'virtual void HealReceived(Unit*,uint32&){}' 'virtual void OnSpellClick(Unit*){}virtual void HealReceived(Unit*,uint32&){}'
$hostSource=Adapt $hostSource 'void HandleGameObject(ObjectGuid,bool){}' 'void HandleGameObject(ObjectGuid,bool){}void ProcessEvent(Unit*,uint32 event){assert(event==49816);++nativeInstanceEvents;}'
$hostSource=Adapt $hostSource 'virtual ~Unit()=default;' 'virtual uint32 GetMapId()const{return nativeMapId;}uint32 nativeMapId=1600;ZoneScript* GetZoneScript(){return nullptr;}virtual ~Unit()=default;'
$hostSource=Adapt $hostSource 'void UpdateAchievementCriteria(uint32,uint32);' 'void UpdateAchievementCriteria(uint32,uint32,uint32=1);'
$hostSource=Adapt $hostSource 'struct GO{ObjectGuid GetGUID()const{return {};}};' 'struct GO:Unit{ObjectGuid GetGUID()const{return {};}};'
$hostSource=Adapt $hostSource 'Creature* erunak=nullptr;' 'Creature* grash=nullptr;Creature* erunak=nullptr;'
$hostSource=Adapt $hostSource 'return id==102826?erunak:' 'return id==105298?grash:id==102826?erunak:'
$hostSource=Adapt $hostSource 'bool preDelete=false,m_AI_locked=false;' @'
uint32 poses=0,animations=0,emotes=0;std::function<void()> onEmote;
void SetByteValue(uint32 field,uint32 offset,uint32 value){assert(field==UNIT_FIELD_BYTES_1&&!offset&&!value);++poses;}
void SetAnimKitId(uint32 id){assert(!id);++animations;}
void HandleEmoteCommand(uint32 id){assert(id==53);++emotes;if(onEmote)onEmote();}
void SetFlag(uint32,uint32 flag){flags|=flag;}
CreatureAI* GetDisabledAI()const{return i_disabledAI;}
bool preDelete=false,m_AI_locked=false;
'@
$hostSource=Adapt $hostSource 'struct ObjectAccessor{' @'
struct ObjectAccessor{inline static std::vector<Creature*> creatures;
static Creature* GetCreature(Unit const& source,ObjectGuid id){for(auto actor:creatures)if(actor->GetGUID()==id&&actor->world&&actor->map==source.map&&!actor->preDelete)return actor;return nullptr;}
'@
$oldCriteria=Block $hostSource 'void Player::UpdateAchievementCriteria('
$hostSource=$hostSource.Replace($oldCriteria,@'
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset,uint32 quantity){
 assert((asset==49816||asset==49957)&&quantity==1);
 if(type==73)return;
 assert(type==92);++credits[asset];
 // Controlled scenario service follows the two matching step1 native AND leaves.
 if(scenario.id==1066&&scenario.step==1&&!scenario.rejected.count(asset)){
  ++scenario.achievements.progress[asset==49816?47123:47197].Counter;
  if(scenario.achievements.progress[47123].Counter&&scenario.achievements.progress[47197].Counter)scenario.step=2;
 }
 if(onCredit)onCredit(asset);
}
'@)
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalRescueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native Breaking Bonds rescue regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_rescue_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native rescue implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
