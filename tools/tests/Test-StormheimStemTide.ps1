param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimStemTideData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimBargain.ps1" *> "$PSScriptRoot/../../.codex/stem-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-stem-tide"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-bargain/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/BargainFixture.inc" -Raw
$f=$f.Replace('struct Position {','struct Position {Position()=default;Position(float a,float b,float c,float d):x(a),y(b),z(c),o(d){}')
$f=$f.Replace('struct WorldObject:Position {','struct WorldObject:Position {void GetPlayerListInGrid(std::list<Player*>&,float)const;')
$f=$f.Replace('struct Motion {','struct Motion {std::vector<Position> flights;void MoveSmoothFlyPath(uint32,Position p,float){flights.push_back(p);}')
$f=$f.Replace('struct Creature:Unit {','struct Creature:Unit {bool gravityDisabled=false,canFly=false;void SetDisableGravity(bool b){gravityDisabled=b;}void SetCanFly(bool b){canFly=b;}')
$f=$f.Replace('uint32 GetItemCount(uint32 id){return bags[id];}','uint32 GetItemCount(uint32 id,bool=false){return bags[id];}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {uint32 visibilityUpdates=0;void UpdateVisibilityOf(WorldObject*){++visibilityUpdates;}')
$f=$f.Replace('struct SpellInfo {','struct SpellInfo {uint32 MiscValue=97983;')
$f=$f.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool CanTargetQuestObject(WorldObject* o){return !o->IsQuestObjectExpired();}bool IsPlayer(){return ToPlayer()!=nullptr;}')
$f=$f.Replace('struct GameObjectAI {','struct GameObjectAI {virtual void SetGUID(ObjectGuid const&,int32){}virtual void UpdateAI(uint32){}')
$f=$f.Replace('struct GameObjectTemplate {','struct GameObjectTemplate {struct {uint32 spellFocusType=1887,radius=10;} spellFocus;struct {uint32 spellFocusType=0,radius=0;} UILink;')
$f=$f.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {bool IsWithinDistInMap(WorldObject const* p,float radius){return map==p->map&&GetExactDist(p)<radius+2;}')
$f=$f.Replace('void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}', 'uint64 now=0;std::vector<std::pair<uint64,std::function<void()>>> scheduled;void AddDelayedEvent(uint64 delay,std::function<void()> f){scheduled.emplace_back(now+delay,std::move(f));}void Advance(uint64 delta){now+=delta;for(;;){auto i=std::find_if(scheduled.begin(),scheduled.end(),[&](auto const& e){return e.first<=now;});if(i==scheduled.end())break;auto action=std::move(i->second);scheduled.erase(i);action();}}')
[IO.File]::WriteAllText("$out/StemFixture.inc",$f)
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectKillCredit(');$b=$effects.IndexOf('void Spell::EffectQuestFail(',$a);[IO.File]::WriteAllText("$out/SpellGroupCredit.inc",$effects.Substring($a,$b-$a))
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spell.IndexOf('bool WorldObjectSpellTargetCheck::operator()(WorldObject* target)');$b=$spell.IndexOf('    if (res != SPELL_CAST_OK)',$a);[IO.File]::WriteAllText("$out/NativeConditionAdmission.inc",$spell.Substring($a,$b-$a)+"    return res == SPELL_CAST_OK;`n}`n")
$grid=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw
$a=$grid.IndexOf('GameObjectFocusCheck::GameObjectFocusCheck(');$b=$grid.IndexOf('NearestGameObjectFishingHole::NearestGameObjectFishingHole(',$a);[IO.File]::WriteAllText("$out/NativeFocusCheck.inc",$grid.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_stem_tide.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','');[IO.File]::WriteAllText("$out/StemTide.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_stem_tide()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimStemTideTest.cpp`" /Fe`"$out/StemTide.exe`" /Fo`"$out/StemTide.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/StemTide.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Stem production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_stem_marker','go_stormheim_stem_portal','spell_stormheim_stem_horn','player_stormheim_stem_tide','Stem the Tide: ignored generic portal credit','Stem the Tide: ignored generic portal talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1;if(!$relative){throw "Missing actual $name link input"};$lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative));if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name input"}}
 'PASS: canonical Release Stem registrations/guards and actual scripts/game link inputs.'
}
