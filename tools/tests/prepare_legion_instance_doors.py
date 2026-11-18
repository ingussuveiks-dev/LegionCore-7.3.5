"""Extract actual scoped arrays, numeric header constants and native door methods."""
import argparse
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCOPES = {
    1456: "EyeofAzshara", 1458: "NeltharionsLair", 1466: "DarkHeartThicket",
    1477: "HallsofValor", 1492: "MawofSouls", 1493: "VaultoftheWardens",
    1501: "BlackRookHoldDungeon", 1516: "TheArcway", 1544: "VioletHoldLegion",
    1571: "CourtofStars", 1651: "ReturnToKarazhan", 1677: "CathedralEternalNight",
    1753: "TheSeatOfTheTriumvirate",
}


def block(text, marker):
    start = text.index(marker)
    pos = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[pos] == "{") - (text[pos] == "}")
        pos += 1
    return text[start:pos]


def enums(text):
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    result = {}
    for body in re.findall(r"enum\s+\w+\s*\{([^{}]*)\}", text):
        value = -1
        for item in body.split(","):
            item = item.strip()
            if not item:
                continue
            parts = item.split("=", 1)
            if len(parts) == 2:
                expr = parts[1].strip()
                assert re.fullmatch(r"[\w\s()+|<>&~xX-]+", expr), expr
                value = eval(expr, {"__builtins__": {}}, result)
            else:
                value += 1
            result[parts[0].strip()] = value
    return result


def prepare(out, baseline=None):
    output, scopes, manifest = [], [], []
    for map_id, folder in SCOPES.items():
        directory = ROOT / "src/server/scripts/Legion" / folder
        source_path, = directory.glob("instance*.cpp")
        source = (subprocess.check_output(["git", "show", f"{baseline}:{source_path.relative_to(ROOT).as_posix()}"], cwd=ROOT).decode("utf-8")
                  if baseline else source_path.read_text(encoding="utf-8-sig"))
        if "DoorData const doorData[]" not in source:
            manifest.append({"map": map_id, "table": False})
            continue
        header, = [directory / name for name in re.findall(r'#include "([^"]+)"', source) if (directory / name).exists()]
        symbols = enums(header.read_text(encoding="utf-8-sig"))
        actual = block(source, "DoorData const doorData[]") + ";"
        needed = set(re.findall(r"\b(?:GO_|DATA_)\w+", actual))
        definitions = "\n".join(f"constexpr uint32 {name}={symbols[name]};" for name in sorted(needed))
        output.append(f"namespace D{map_id} {{\n{definitions}\n{actual}\n}}")
        scopes.append(f"{{{map_id},{symbols['MAX_ENCOUNTER']},D{map_id}::doorData,sizeof(D{map_id}::doorData)/sizeof(DoorData)}}")
        constants = dict(symbols, DOOR_TYPE_ROOM=0, DOOR_TYPE_PASSAGE=1, DOOR_TYPE_SPAWN_HOLE=2,
                         BOUNDARY_NONE=0, BOUNDARY_N=1, BOUNDARY_S=2, BOUNDARY_E=3,
                         BOUNDARY_W=4, BOUNDARY_NE=5, BOUNDARY_NW=6, BOUNDARY_SE=7, BOUNDARY_SW=8)
        clean = re.sub(r"//[^\n]*", "", actual)
        rows = [[constants[x.strip()] if x.strip() in constants else int(x.strip(), 0)
                 for x in row.split(",")] for row in re.findall(r"\{([^{}]+)\}", clean)]
        manifest.append({"map": map_id, "table": True, "source": source_path.relative_to(ROOT).as_posix(),
                         "bosses": symbols["MAX_ENCOUNTER"], "terminated": bool(re.search(r"\{\s*0\s*,", actual)),
                         "script": re.search(r"class (instance_\w+)", source)[1],
                         "implementation": re.search(r"struct (\w+) : public InstanceScript", source)[1],
                         "rows": rows})
    output.append("static DoorScope const doorScopes[]={" + ",\n".join(scopes) + "};")
    (out / "LegionDoorArrays.inc").write_text("\n".join(output), encoding="utf-8")
    core = (ROOT / "src/server/game/Instances/InstanceScript.cpp").read_text(encoding="utf-8-sig")
    for name in ("LoadDoorData", "UpdateDoorState"):
        (out / f"{name}Actual.inc").write_text(block(core, f"void InstanceScript::{name}("), encoding="utf-8")
    creature = (ROOT / "src/server/game/Entities/Creature/Creature.h").read_text(encoding="utf-8-sig")
    (out / "DoorBoundaryActual.inc").write_text(block(creature, "enum BoundaryType") + ";", encoding="utf-8")
    (out / "door-arrays.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--baseline")
    args = parser.parse_args()
    prepare(args.output, args.baseline)
