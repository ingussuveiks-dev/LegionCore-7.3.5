param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimWingsData.ps1"
& "$PSScriptRoot/Test-StormheimJudgment.ps1" *> "$repo/.codex/wings-judgment-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-wings"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-judgment/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/JudgmentFixture.inc" -Raw
$fixture=$fixture.Replace('uint32 open=1691,questID=38811;','uint32 open=1690,questID=0,spell=193637;')
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {bool fly=false,gravity=true;FakeSpline spline;FakeSpline* movespline=&spline;void SetCanFly(bool b){fly=b;}void SetDisableGravity(bool b){gravity=!b;}bool ShouldHideFor(ObjectGuid id){return hidden.count(id);}std::set<uint32> GetPhases(){return {};}')
$fixture=$fixture.Replace('struct HeightMap {','struct HeightMap {bool clear=true;bool isInLineOfSight(float,float,float,float,float,float,std::set<uint32> const&,VMAP::ModelIgnoreFlags){return clear;}')
$fixture=$fixture.Replace('struct GameObjectAI {','struct GameObjectAI {GameObject* go;explicit GameObjectAI(GameObject* g=nullptr):go(g){if(g)g->ai=this;}')
$fixture=$fixture.Replace('void UpdateVisibilityForPlayer(){}','void UpdateVisibilityForPlayer(){}')
[IO.File]::WriteAllText("$out/JudgmentFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/StormheimJudgmentTest.cpp" -Raw
$prefix=$prefix.Substring(0,$prefix.IndexOf('uint64 nextGuid='))
$prefix=$prefix.Replace('"Judgment.inc"','"Wings.inc"').Replace('spell_stormheim_judgment_opening','spell_stormheim_wings_opening').Replace('"go_stormheim_judgment"','"go_stormheim_wings"').Replace('id==1691','id==1690')
$extra=@'
#include <vector>
#include <cstdint>
namespace VMAP {enum class ModelIgnoreFlags {Nothing};}
namespace G3D {struct Vector3 {float x,y,z;Vector3(float a,float b,float c):x(a),y(b),z(c){}};}
struct FakeSpline {bool finished=false;uint32_t launches=0;std::vector<G3D::Vector3> path;bool Finalized(){return finished;}};
'@
$prefix=$prefix.Replace('#include "JudgmentFixture.inc"',$extra+"`n"+'#include "JudgmentFixture.inc"'+@'

constexpr uint32 UNIT_FLAG_NON_ATTACKABLE=2;
namespace Movement {using PointsArray=std::vector<G3D::Vector3>;struct MoveSplineInit {Creature& actor;PointsArray path;bool fly=false;float speed=0;explicit MoveSplineInit(Creature& a):actor(a){}void MovebyPath(PointsArray const& p){path=p;}void SetFly(){fly=true;}void SetVelocity(float s){speed=s;}void Launch(){assert(fly&&actor.fly&&!actor.gravity&&speed==8&&path.size()==3);actor.spline.path=path;actor.spline.finished=false;++actor.spline.launches;}};}
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& out,WorldObject*,uint32 entry,float){for(auto const& row:objects)if(row.second->entry==entry&&row.second->world)out.push_back(row.second);}
'@)
$prefix=$prefix.Replace('virtual void OnLogout(Player*){}};','virtual void OnLogout(Player*){}};')
[IO.File]::WriteAllText("$out/WingsFixture.inc",$prefix)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_wings.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/Wings.inc",'#include "'+$repo+'/src/server/scripts/Legion/stormheim_wings_departure.h"'+"`n"+$source.Substring(0,$source.IndexOf('void AddSC_stormheim_wings()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimWingsTest.cpp`" /Fe`"$out/Wings.exe`" /Fo`"$out/Wings.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Wings.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Wings release native regression failed'}
if($VerifyReleaseBinary){$exe="$repo/build-extractors/bin/Release/worldserver.exe";$binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe));foreach($marker in @('go_stormheim_wings','spell_stormheim_wings_opening','player_stormheim_wings','Wings of Liberty: private release after completed opening','Wings of Liberty: native Opening dispatch')){if(!$binary.Contains($marker)){throw "Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj";$release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1;if(!$relative){throw "Missing actual $name link input"};$lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative));if((Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Stale $name final link"}}
 'PASS: deployed Release registration/dispatch marker and actual game/scripts final-link libraries.'
}
