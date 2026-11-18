"""Inventory numeric spell references and declared script loaders in the13-map scope."""
import json
from pathlib import Path
import re
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tests"))
from prepare_legion_instance_doors import ROOT, SCOPES

def prepare(output):
    refs, names = [], {}
    for map_id, folder in SCOPES.items():
        for path in sorted((ROOT/"src/server/scripts/Legion"/folder).glob("*.cpp")):
            source=re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(encoding="utf-8-sig"), flags=re.S)
            relative=path.relative_to(ROOT).as_posix()
            for name, value in re.findall(r"\b(SPELL_\w+)\s*=\s*(\d+)\b",source):
                if int(value): refs.append({"map":map_id,"source":relative,"symbol":name,"spell":int(value)})
            declared=re.findall(r'SpellScriptLoader\s*\(\s*"([^"]+)"',source)
            declared+=re.findall(r"Register(?:Spell|Aura)Script\s*\(\s*(\w+)\s*\)",source)
            for name in declared: names.setdefault(name,[]).append(relative)
    output.mkdir(parents=True,exist_ok=True)
    payload={"scope":"13 Legion level110 dungeons;numeric SPELL_ references and declared loaders only","references":refs,"loaders":names}
    (output/"mechanics-source.json").write_text(json.dumps(payload,indent=2)+"\n",encoding="utf-8")
    rows=["{%d,%s,%s,%d}"%(r["map"],json.dumps(r["source"]),json.dumps(r["symbol"]),r["spell"]) for r in refs]
    text="static MechanicSpellRef const mechanicRefs[]={\n"+",\n".join(rows)+"\n};\n"
    text+="static char const* const mechanicLoaders[]={"+",".join(json.dumps(n) for n in sorted(names))+"};\n"
    (output/"LegionMechanicsRefs.inc").write_text(text,encoding="utf-8")
    print(f"SOURCE numeric references={len(refs)} loader names={len(names)}")

if __name__=="__main__": prepare(Path(sys.argv[1]))
