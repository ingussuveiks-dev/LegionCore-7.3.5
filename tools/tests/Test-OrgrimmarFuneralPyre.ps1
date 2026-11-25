param([switch]$Baseline)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$path = 'src/server/game/Entities/GameObject/GameObject.cpp'
$mode = if ($Baseline) { 'baseline' } else { 'repaired' }
$work = "$repo/.codex/orgrimmar-funeral-pyre-20261125/$mode"
New-Item -ItemType Directory -Force -Path $work | Out-Null

$source = if ($Baseline) {
    (git -C $repo show "c6d332aec:$path") -join [Environment]::NewLine
    if ($LASTEXITCODE) { throw 'Could not read the baseline source' }
} else {
    Get-Content -LiteralPath "$repo/$path" -Raw
}
$case = [regex]::Match($source, '(?ms)^        case GAMEOBJECT_TYPE_PHASEABLE_MO:\s*(.*?)^            break;')
if (!$case.Success) { throw 'Could not find the phaseable model creation case' }
[IO.File]::WriteAllText("$work/PhaseableModel.inc", $case.Groups[1].Value)

$test = @'
#include <array>
#include <cstdint>
#include <cstdio>

struct Map
{
    std::uint32_t id;
    std::uint32_t GetId() const { return id; }
};

struct GameObject
{
    bool active = false;
    bool worldObject = false;
    void setActive(bool value) { active = value; }
    void SetWorldObject(bool value) { worldObject = value; }

    void CreatePhaseableModel(std::uint32_t name_id, std::uint64_t m_DBTableGuid,
        Map const* map, bool MaxVisible)
    {
        // The baseline unconditionally activates the object.
        (void)name_id;
        (void)m_DBTableGuid;
        (void)map;
        (void)MaxVisible;
#include "PhaseableModel.inc"
    }
};

int main()
{
    struct Case
    {
        char const* name;
        std::uint32_t entry;
        std::uint64_t spawn;
        Map map;
        bool maxVisible;
        bool active;
    };

    constexpr std::array cases{
        Case{"local Orgrimmar pyre", 258064, 131298, {1}, false, EXPECT_BASELINE},
        Case{"explicit maximum visibility", 258064, 131298, {1}, true, true},
        Case{"same template with a different spawn", 258064, 131299, {1}, false, true},
        Case{"dynamic object without a spawn", 258064, 0, {1}, false, true},
        Case{"different entry", 258065, 131298, {1}, false, true},
        Case{"different map", 258064, 131298, {0}, false, true}
    };

    for (auto const& test : cases)
    {
        GameObject object;
        object.CreatePhaseableModel(test.entry, test.spawn, &test.map, test.maxVisible);
        if (object.active != test.active || !object.worldObject)
        {
            std::fprintf(stderr, "%s: active=%d, worldObject=%d\n", test.name,
                object.active, object.worldObject);
            return 1;
        }
    }

    std::puts(EXPECT_BASELINE
        ? "PASS counterfactual: the old case forces the pyre active"
        : "PASS repair: the local pyre is inactive, and all six cases retain world-object status");
}
'@
[IO.File]::WriteAllText("$work/Test.cpp", $test)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler was not found' }
$expected = if ($Baseline) { 1 } else { 0 }
$build = @(
    '@echo off'
    ('call "{0}/VC/Auxiliary/Build/vcvars64.bat" >nul' -f $vs)
    'if errorlevel 1 exit /b %errorlevel%'
    ('cl /nologo /W4 /WX /std:c++17 /EHsc /DEXPECT_BASELINE={0} /Fo"{1}/Test.obj" /Fe"{1}/Test.exe" "{1}/Test.cpp"' -f $expected, $work)
    'exit /b %errorlevel%'
)
[IO.File]::WriteAllLines("$work/build-test.cmd", $build)
& "$work/build-test.cmd"
if ($LASTEXITCODE) { throw 'Pyre regression compilation failed' }
& "$work/Test.exe"
if ($LASTEXITCODE) { throw 'Pyre regression failed' }
