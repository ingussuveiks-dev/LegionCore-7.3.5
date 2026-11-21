"""Read-only Legion 7.3.5 quest evidence. Flags are investigation candidates."""
import argparse
from collections import Counter, defaultdict
import csv
from functools import cache
import hashlib
import json
from pathlib import Path
import re
import struct

from audit_spell_visuals import Table


ROOT = Path(__file__).resolve().parents[2]
OBJECTIVES = ("kill_or_credit", "item", "gameobject", "talk", "currency", "learn_spell",
              "min_reputation", "max_reputation", "money", "player_kills", "area_trigger",
              "pet_trainer", "defeat_battle_pet", "pet_battle_victories", "criteria_tree",
              "task_in_zone", "have_currency", "obtain_currency")
LOOT_SOURCE = {"creature": 1, "fishing": 3, "object": 4, "item": 5, "mail": 6,
               "pickpocket": 8, "reference": 10, "skinning": 11, "spell": 12, "world": 31}
SPAWN_SAMPLE = 6


def number(row, key, default=0):
    value = row.get(key)
    return int(value) if value not in (None, "", "NULL") else default


def group(rows, key):
    result = defaultdict(list)
    for row in rows:
        result[key(row)].append(row)
    return result


def integers(row, *keys):
    return [number(row, key) for key in keys]


def signed(value, bits):
    limit = 1 << bits
    return ((value + limit // 2) % limit) - limit // 2


def row_values(row, columns):
    return [row.get(column) for column in columns.split()]


def file_hash(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


class Evidence:
    def __init__(self, path, dbc, binary):
        self.path = path
        self.dbc = dbc
        self.binary = binary
        self.hashes = {}
        self.deleted = defaultdict(set)
        for row in self.rows("hotfix_lookup"):
            ids = self.deleted[number(row, "TableHash")]
            (ids.add if number(row, "Deleted") else ids.discard)(number(row, "RecordID"))

    def rows(self, name):
        path = self.path / (name + ".tsv")
        self.hashes[f"sql/{path.name}"] = file_hash(path)
        with path.open(encoding="utf-8-sig", newline="") as stream:
            yield from csv.DictReader(stream, delimiter="\t", quoting=csv.QUOTE_NONE)

    def table(self, name, index=-1, layout=None):
        path = self.dbc / (name + ".db2")
        table = Table(path, index, layout)
        self.hashes[f"db2/{path.name}"] = table.sha256
        return table

    def native_rows(self, name, read, hotfix=None, hotfix_read=None, index=-1, layout=None):
        table = self.table(name, index, layout)
        records = {key: read(table, i) for i, key in enumerate(table.ids)}
        for key, original in table.copy_pairs:
            records[key] = records[original]
        if hotfix:
            records.update((number(r, "ID"), hotfix_read(r)) for r in self.rows(hotfix))
        return {key: value for key, value in records.items()
                if key not in self.deleted[table.u32(20)]}


def sparse_items(evidence):
    path = evidence.dbc / "ItemSparse.db2"
    data = path.read_bytes()
    evidence.hashes["db2/ItemSparse.db2"] = hashlib.sha256(data).hexdigest()
    count, fields = struct.unpack_from("<II", data, 4)
    table_hash, layout, minimum, maximum = struct.unpack_from("<4I", data, 20)
    flags = struct.unpack_from("<H", data, 44)[0]
    catalog = struct.unpack_from("<I", data, 60)[0]
    if data[:4] != b"WDC1" or layout != 0x4007DE16 or fields != 64 or not flags & 1:
        raise ValueError("Unexpected Legion ItemSparse layout")
    meta = (ROOT / "src/server/game/DataStores/DB2Metadata.h").read_text(encoding="utf-8")
    types, array_sizes = re.search(
        r'struct ItemSparseMeta.+?types = "([^"]+)".+?arraySizes\[64\] = \{([^}]+)\}',
        meta, re.S).groups()
    sizes = [int(n) for n in array_sizes.split(",")]
    widths = [4 - struct.unpack_from("<h", data, 84 + 4 * j)[0] // 8 for j in range(fields)]
    result = {}
    physical_records = set()
    for item in range(minimum, maximum + 1):
        start, length = struct.unpack_from("<IH", data, catalog + (item - minimum) * 6)
        if not start or not length:
            continue
        physical_records.add((start, length))
        offset, end = start, start + length
        for index, (kind, size, width) in enumerate(zip(types, sizes, widths)):
            if kind == "s":
                offset = data.index(0, offset, end) + 1
                if index == 1:
                    name = data[start + widths[0]:offset - 1].decode("utf-8")
            else:
                if index in (31, 47):
                    value = int.from_bytes(data[offset:offset + width], "little", signed=index == 47)
                    if index == 31:
                        quest = value
                    else:
                        result[item] = [name, quest, value]
                        break
                offset += size * (4 if kind == "f" else width)
    if len(physical_records) != count:
        raise ValueError(f"ItemSparse catalog declared {count} records, found {len(physical_records)}")
    result.update((number(r, "ID"), [r["Display"], *integers(r, "StartQuestID", "RequiredLevel")])
                  for r in evidence.rows("hotfix_item_sparse"))
    return {key: value for key, value in result.items() if key not in evidence.deleted[table_hash]}


def cpp_evidence(quest_ids, script_names, binary):
    quest_refs = defaultdict(list)
    names = defaultdict(list)
    paths = sorted((ROOT / "src/server/scripts").rglob("*.cpp"))
    paths.extend(ROOT / "src/server/game" / name for name in (
        "Entities/Player/Player.cpp", "Entities/GameObject/GameObject.cpp",
        "Globals/QuestData.cpp", "Handlers/QuestHandler.cpp", "Spells/SpellEffects.cpp"))
    for path in paths:
        rel = path.relative_to(ROOT).as_posix()
        for line_number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            if re.search(r'quest|KilledMonsterCredit|KillCreditGO|GroupEventHappens', line, re.I):
                for value in set(re.findall(r'(?<![\w.])\d{1,6}(?=[uUlL]?\b)', line)):
                    if int(value) in quest_ids:
                        quest_refs[int(value)].append([rel, line_number, line.strip()[:240]])
            for token in set(re.findall(r'[A-Za-z_][\w]*', line)) & script_names:
                if ("class " in line or "struct " in line or "Register" in line or
                        "new " in line or f'"{token}"' in line):
                    if len(names[token]) < 16:
                        names[token].append([rel, line_number, line.strip()[:240]])
    release = binary.read_bytes()
    return quest_refs, {name: {"references": names[name],
                               "name_string_in_release": (name + "\0").encode() in release}
                        for name in sorted(script_names)}


class Audit:
    def __init__(self, evidence):
        self.e = e = evidence
        self.quests = {number(r, "ID"): r for r in e.rows("quests")}
        self.addons = {number(r, "ID"): r for r in e.rows("addons")}
        self.primary_scope = {q for q, r in self.quests.items()
                              if any(1 <= number(r, k) <= 20 for k in ("QuestLevel", "MinLevel"))}
        self.zero_minimum = {q for q, r in self.quests.items() if number(r, "MinLevel") == 0} - self.primary_scope
        self.scope = self.primary_scope | self.zero_minimum
        self.disabled = {number(r, "entry"): r["comment"] for r in e.rows("disabled")}
        self.objectives = group((r for r in e.rows("objectives") if number(r, "QuestID") in self.scope),
                                lambda r: number(r, "QuestID"))
        self.native_objectives = group(
            e.native_rows("QuestObjective",
                          lambda t, i: [t.ids[i], t.parents.get(i, 0), *[t.value(i, j) for j in (3, 2, 1, 5, 6)]],
                          "hotfix_quest_objective",
                          lambda r: integers(r, "ID", "QuestID", "Type", "ObjectID", "Amount", "StorageIndex", "Flags"),
                          layout=0xDD995180).values(), lambda r: r[1])
        self.relations = group((r for r in e.rows("relations") if number(r, "quest") in self.scope),
                               lambda r: number(r, "quest"))
        self.creatures = {number(r, "entry"): r for r in e.rows("creatures")}
        self.objects = {number(r, "entry"): r for r in e.rows("objects")}
        self.native_objects = e.native_rows("GameObjects", lambda t, i: [t.value(i, 10), t.value(i, 6)],
                                            "hotfix_gameobjects", lambda r: integers(r, "TypeID", "DisplayID"),
                                            index=11, layout=0x597E8643)
        self.displays = e.native_rows("GameObjectDisplayInfo", lambda t, i: True)
        self.criteria = e.native_rows("CriteriaTree", lambda t, i: True, "hotfix_criteria_tree", lambda r: True)
        self.areas = e.native_rows("AreaTable", lambda t, i: "", "hotfix_area", lambda r: r["AreaName"])
        self.area_triggers = e.native_rows("AreaTrigger", lambda t, i: True, index=14)
        self.quest_poi = group(e.rows("quest_poi"), lambda r: number(r, "QuestID"))
        self.item_basics = e.native_rows("Item", lambda t, i: t.value(i, 1), "hotfix_item",
                                         lambda r: number(r, "ClassID"), layout=0x0DFCC83D)
        self.items = sparse_items(e)
        self.item_starters = group(((item, data) for item, data in self.items.items()
                                   if item in self.item_basics and data[1] in self.scope), lambda pair: pair[1][1])
        self.spells = e.native_rows("Spell", lambda t, i: True, "hotfix_spell", lambda r: True, layout=0x2273DFFF)
        self.casting_requirements = {
            spell: [area, focus] for spell, area, focus in e.native_rows(
                "SpellCastingRequirements", lambda t, i: [t.value(i, 0), signed(t.value(i, 2), 16), t.value(i, 3)],
                layout=0xD8B56E5D).values()
        }
        self.items_on_use = group((r for r in e.native_rows(
            "ItemEffect", lambda t, i: [t.parents.get(i, 0), *[t.value(i, c) for c in (0, 7)],
                                       signed(t.value(i, 3), 16)],
            "hotfix_item_effect", lambda r: integers(r, "ItemID", "SpellID", "TriggerType", "Charges"),
            layout=0xA390FA40).values() if r[1] and r[2] in (0, 5)), lambda r: r[0])
        self.effects = group(e.native_rows(
            "SpellEffect", lambda t, i: [t.parents.get(i, 0), *[t.value(i, c) for c in (1, 3, 5, 12, 16)],
                                         *t.value(i, 26), *t.value(i, 28)],
            "hotfix_spell_effect", lambda r: integers(r, "SpellID", "Effect", "EffectIndex", "DifficultyID",
                                                     "EffectItemType", "EffectTriggerSpell", "EffectMiscValue1",
                                                     "EffectMiscValue2", "ImplicitTarget1", "ImplicitTarget2"),
            index=0, layout=0x3244098B).values(), lambda r: r[0])
        self.quest_spells = defaultdict(list)
        self.item_makers = defaultdict(list)
        self.credit = defaultdict(list)
        self.summons = defaultdict(list)
        for spell, effects in self.effects.items():
            for row in effects:
                _, kind, _, _, item, _, misc, _, _, _ = row
                if kind in (16, 150) and misc in self.scope:
                    self.quest_spells[misc].append(row)
                if item and kind in (24, 157, 206):
                    self.item_makers[item].append(row)
                if misc and kind in (90, 134):
                    self.credit[misc].append(row)
                if misc and kind in (28, 76, 104):
                    self.summons[("npc" if kind == 28 else "go", misc)].append(["spell", spell, kind])

        self.smart = group(e.rows("smart"), lambda r: (number(r, "source_type"), number(r, "entryorguid")))
        self.smart_quest = defaultdict(list)
        self.smart_credit = defaultdict(list)
        self.smart_items = defaultdict(list)
        for (source, entry), rows in self.smart.items():
            for row in rows:
                event, event_arg, action, arg = integers(row, "event_type", "event_param1", "action_type", "action_param1")
                if (event in (19, 20) and event_arg in self.scope):
                    self.smart_quest[event_arg].append(row)
                if action in (6, 7, 15, 26, 211, 223) and arg in self.scope and arg != event_arg:
                    self.smart_quest[arg].append(row)
                if action in (12, 50, 220) and arg:
                    self.summons[("go" if action == 50 else "npc", arg)].append(["smart", source, entry, number(row, "id")])
                if action == 33 and arg:
                    self.smart_credit[arg].append(row)
                if action == 56 and arg:
                    self.smart_items[arg].append(row)
        for row in e.rows("summon_groups"):
            self.summons[("npc", number(row, "entry"))].append(
                ["summon_group", *integers(row, "summonerType", "summonerId", "groupId")])
        self.conditions = group(e.rows("conditions"), lambda r: tuple(integers(
            r, "SourceTypeOrReferenceId", "SourceGroup", "SourceEntry", "SourceId")))
        self.spell_conditions = defaultdict(list)
        for (source, _, spell, _), rows in self.conditions.items():
            if source in (13, 17):
                self.spell_conditions[spell].extend(rows)
        self.spell_checks = group(e.rows("spell_checks"), lambda r: number(r, "spellId"))
        self.spell_scripts = group(e.rows("spell_scripts"), lambda r: abs(number(r, "spell_id")))
        self.spell_click = group(e.rows("spell_click"), lambda r: number(r, "npc_entry"))
        self.spell_area = defaultdict(list)
        self.spell_area_by_spell = defaultdict(list)
        for row in e.rows("spell_area"):
            self.spell_area_by_spell[number(row, "spell")].append(row)
            for quest in set(integers(row, "quest_start", "quest_end")) & self.scope:
                self.spell_area[quest].append(row)
        self.phase_quests = defaultdict(list)
        self.phase_defs = {tuple(integers(r, "zoneId", "entry")): r for r in e.rows("phase_definitions")}
        for (source, zone, entry, _), rows in self.conditions.items():
            if source == 23:
                for quest in {number(r, "ConditionValue1") for r in rows
                              if number(r, "ConditionTypeOrReference") in (8, 9, 14, 28, 41)} & self.scope:
                    self.phase_quests[quest].append([self.phase_defs.get((zone, entry)), rows])
        self.object_actions = group(e.rows("object_actions"), lambda r: number(r, "entry"))
        self.object_visuals = group(e.rows("object_visuals"), lambda r: number(r, "questID"))
        self.event_relations = group(
            (*e.rows("npc_quest_events"), *e.rows("go_quest_events")), lambda r: number(r, "quest"))
        self.area_scripts = group(e.rows("area_scripts"), lambda r: number(r, "entry"))
        self.start_scripts = group(e.rows("db_quest_start"), lambda r: number(r, "id"))
        self.end_scripts = group(e.rows("db_quest_end"), lambda r: number(r, "id"))
        self.db_quest_credit = defaultdict(list)
        for table in ("db_quest_start", "db_quest_end", "db_event_scripts", "db_object_scripts"):
            for row in e.rows(table):
                command, value = integers(row, "command", "datalong")
                if command == 7 and value in self.scope:
                    self.db_quest_credit[value].append([table, row])
                elif command == 10 and value:
                    self.summons[("npc", value)].append([table, number(row, "id")])

        self.reverse_credit = defaultdict(list)
        self.npc_loot = defaultdict(list)
        self.go_loot = defaultdict(list)
        self.go_quest_items = defaultdict(list)
        self.focus_objects = defaultdict(list)
        for npc, row in self.creatures.items():
            for alias in set(integers(row, "KillCredit1", "KillCredit2")) - {0}:
                self.reverse_credit[alias].append(npc)
            for kind, field in (("creature", "lootid"), ("pickpocket", "pickpocketloot"), ("skinning", "skinloot")):
                if loot := number(row, field):
                    self.npc_loot[kind, loot].append(npc)
        for obj, row in self.objects.items():
            if loot := self.object_loot_key(row):
                self.go_loot[loot].append(obj)
            if number(row, "type") == 8:
                self.focus_objects[number(row, "Data0")].append(obj)
            for item in set(integers(row, *(f"questItem{i}" for i in range(1, 7)))) - {0}:
                self.go_quest_items[item].append(obj)
        self.spawns = {}
        for kind, prefix in (("npc", "creature"), ("go", "object")):
            events = group(e.rows(f"{prefix}_events"), lambda r: number(r, "guid"))
            pools = group(e.rows(f"{prefix}_pools"), lambda r: number(r, "guid"))
            self.spawns[kind] = group(e.rows(f"{prefix}_spawns"), lambda r: number(r, "id"))
            for rows in self.spawns[kind].values():
                for row in rows:
                    guid = number(row, "guid")
                    row["events"] = [number(r, "eventEntry") for r in events.get(guid, [])]
                    row["pools"] = [number(r, "pool_entry") for r in pools.get(guid, [])]
        self.loot = defaultdict(list)
        self.references = defaultdict(list)
        for kind in (*LOOT_SOURCE, "zone"):
            for row in e.rows(f"loot_{kind}"):
                item, currency, reference = integers(row, "Item", "Currency", "Reference")
                if reference and float(row["Chance"]) >= 0:
                    self.references[abs(reference)].append((kind, row))
                if item and not currency:
                    self.loot[item].append((kind, row))
        self.vendors = group(e.rows("vendor_items"), lambda r: number(r, "item"))
        self.rewards = defaultdict(list)
        for quest, row in self.quests.items():
            for item in set(integers(row, *(f"RewardItem{i}" for i in range(1, 5)))) - {0}:
                self.rewards[item].append(quest)
        self.cpp_names = set()

    @staticmethod
    def object_loot_key(row):
        kind = number(row, "type")
        return number(row, "Data0" if kind == 51 else "Data1") if kind in (3, 25, 50, 51) else 0

    def record_script(self, name):
        if name not in (None, "", "NULL", "SmartAI", "SmartGameObjectAI"):
            self.cpp_names.add(name)
            return name
        return None

    @cache
    def actor(self, kind, entry):
        if kind == "go":
            record = self.objects.get(entry)
            if not record and entry in self.native_objects:
                return {"entry": entry, "db2_only_template": self.native_objects[entry],
                        **self.spawn_info(kind, entry)}
        else:
            record = self.creatures.get(entry)
        info = {"entry": entry, "template_present": bool(record), **self.spawn_info(kind, entry)}
        if not record:
            return info
        info["script"] = self.record_script(record["ScriptName"])
        info["ai"] = record["AIName" if kind == "npc" else "AiName"]
        info["smart_rows"] = len(self.smart.get((0 if kind == "npc" else 1, entry), []))
        spawn_rows = self.spawns[kind].get(entry, [])
        source = 0 if kind == "npc" else 1
        info["spawn_specific_smart_rows"] = sum(len(self.smart.get((source, -number(r, "guid")), [])) for r in spawn_rows)
        if kind == "npc":
            info.update(name=record["Name1"], npc_flags=number(record, "npcflag"),
                        unit_flags=number(record, "unit_flags"),
                        levels=integers(record, "minlevel", "maxlevel", "LevelScalingMin", "LevelScalingMax"),
                        spellclick=[{"spell": number(r, "spell_id"), "conditions": self.conditions.get(
                            (18, entry, number(r, "spell_id"), 0), [])} for r in self.spell_click.get(entry, [])])
        else:
            kind_id = number(record, "type")
            quest_column = {3: "Data8", 5: "Data5", 8: "Data4", 10: "Data1", 50: "Data8"}.get(kind_id)
            info.update(name=record["name"], type=kind_id, flags=number(record, "flags"),
                        display=number(record, "displayId"), loot=self.object_loot_key(record),
                        quest_gate=number(record, quest_column) if quest_column else 0,
                        actions=self.object_actions.get(entry, []))
        return info

    def spawn_info(self, kind, entry):
        spawns = self.spawns[kind].get(entry, [])
        return {"spawn_count": len(spawns), "maps": sorted({number(r, "map") for r in spawns}),
                "zones": sorted({number(r, "zoneId") for r in spawns}),
                "static_unphased_ungated_count": sum(not (r["events"] or r["pools"] or
                    r["PhaseId"] not in ("0", "") or number(r, "phaseMask") not in (0, 1) or
                    (kind == "go" and number(r, "spawntimesecs") < 0)) for r in spawns),
                "spawn_sample": [[*row_values(r, "guid map zoneId areaId spawnMask PhaseId phaseMask position_x position_y position_z"),
                                  r["events"], r["pools"]] for r in spawns[:SPAWN_SAMPLE]],
                "summon_references": self.summons.get((kind, entry), [])}

    @cache
    def loot_routes(self, item):
        def parents(kind, row, visited, through):
            entry = number(row, "Entry")
            if kind == "reference":
                if entry not in visited:
                    for parent_kind, parent in self.references.get(entry, []):
                        yield from parents(parent_kind, parent, visited | {entry}, [entry, *through])
                return
            actors = self.go_loot.get(entry, []) if kind == "object" else self.npc_loot.get((kind, entry), [])
            yield [kind, entry, list(actors), through, row["Chance"], number(row, "QuestRequired"),
                   number(row, "LootMode"), self.conditions.get((LOOT_SOURCE.get(kind, 0), entry, item, 0), [])]
        routes = []
        for kind, row in self.loot.get(item, []):
            for route in parents(kind, row, set(), []):
                if route[3]:
                    route.append([row["Chance"], number(row, "QuestRequired"), number(row, "LootMode"),
                                  self.conditions.get((10, number(row, "Entry"), item, 0), [])])
                if route not in routes:
                    routes.append(route)
        return routes

    def item(self, entry, quest, role, amount, findings):
        data = self.items.get(entry)
        routes = self.loot_routes(entry)
        vendors = [[number(r, "entry"), number(r, "ExtendedCost"), number(r, "PlayerConditionID")]
                   for r in self.vendors.get(entry, []) if number(r, "type") == 1]
        info = {"entry": entry, "role": role, "amount": amount, "data": data,
                "class": self.item_basics.get(entry), "loot_routes": routes, "vendors": vendors,
                "reward_quests": self.rewards.get(entry, []), "create_item_effects": self.item_makers.get(entry, []),
                "smart_add_item": self.smart_items.get(entry, []), "object_markers": self.go_quest_items.get(entry, [])}
        if entry not in self.item_basics or not data:
            findings.add("item_template_missing")
        if role in ("objective", "starter") and not any((routes, vendors, info["reward_quests"],
                info["create_item_effects"], info["smart_add_item"], entry == number(self.quests[quest], "StartItem"))):
            findings.add("item_acquisition_untraced")
        if role == "objective" and routes and not any(
            (kind not in ("object", "creature", "pickpocket", "skinning") or
             any(self.spawns["go" if kind == "object" else "npc"].get(a) or
                 self.summons.get(("go" if kind == "object" else "npc", a)) for a in actors))
             and (float(chance) >= 0) for kind, _, actors, _, chance, *_ in routes):
            findings.add("loot_source_without_traced_placement")
        if role == "objective":
            loot_objects = {a for route in routes if route[0] == "object" for a in route[2]}
            for route in routes:
                if route[0] == "object" and not route[2] and route[1] in self.objects:
                    alleged_object = self.objects[route[1]]
                    if number(alleged_object, "type") in (3, 25, 50, 51):
                        findings.add("loot_entry_has_no_object_template_link")
            info["object_sources"] = [self.actor("go", a) for a in sorted(loot_objects | set(info["object_markers"]))]
            if any(self.spawns["go"].get(a) for a in loot_objects | set(info["object_markers"])):
                findings.add("static_item_source_visibility_requires_phase_or_script_check")
            if not loot_objects and entry != number(self.quests[quest], "StartItem") and any(
                    self.objects.get(a) and number(self.objects[a], "type") in (3, 50) for a in info["object_markers"]):
                findings.add("object_item_marker_without_linked_loot")
        uses = []
        for _, spell, trigger, charges in self.items_on_use.get(entry, []):
            scripts = [self.record_script(r["ScriptName"]) for r in self.spell_scripts.get(spell, [])]
            checks = self.spell_checks.get(spell, [])
            conditions = self.spell_conditions.get(spell, [])
            area, focus = self.casting_requirements.get(spell, [-1, 0])
            uses.append({"spell": spell, "trigger": trigger, "charges": charges, "spell_present": spell in self.spells,
                         "effects": self.effects.get(spell, []), "conditions": conditions, "checks": checks,
                         "area_rules": self.spell_area_by_spell.get(spell, []),
                         "base_db2_required_area_group": area, "base_db2_spell_focus": focus,
                         "focus_objects": [self.actor("go", a) for a in self.focus_objects.get(focus, [])] if focus else [],
                         "triggered_spells": {r[5]: self.effects.get(r[5], []) for r in self.effects.get(spell, []) if r[5]},
                         "scripts": [name for name in scripts if name]})
            if (role != "objective" or self.item_basics.get(entry) == 12) and not (
                    checks or conditions or scripts or self.spell_area_by_spell.get(spell) or area > 0 or focus):
                findings.add("item_use_target_gate_requires_manual_review")
            if spell not in self.spells:
                findings.add("item_use_spell_missing")
        if uses:
            info["uses"] = uses
        return info

    def quest(self, quest):
        row = self.quests[quest]
        addon = self.addons.get(quest, {})
        findings = set()
        flags, flags_ex = integers(row, "Flags", "FlagsEx")
        special = number(addon, "SpecialFlags")
        result = {"id": quest, "title": row["LogTitle"], "summary": row["LogDescription"],
                  "level": number(row, "QuestLevel"), "minimum": number(row, "MinLevel"),
                  "scaling_max": number(row, "QuestMaxScalingLevel"), "expansion": number(row, "Expansion"),
                  "sort": number(row, "QuestSortID"), "quest_info": number(row, "QuestInfoID"),
                  "race_mask": row["AllowableRaces"], "flags": flags, "flags_ex": flags_ex,
                  "suggested_group": number(row, "SuggestedGroupNum"),
                  "limit_seconds": number(row, "TimeAllowed"), "addon": addon, "disabled": quest in self.disabled}
        if result["disabled"]:
            result["disable_reason"] = self.disabled[quest]
        for value in [*integers(addon, "PrevQuestID", "NextQuestID"), number(row, "RewardNextQuest")]:
            if value and abs(value) not in self.quests:
                findings.add("chain_link_template_missing")
        if number(addon, "PrevQuestID") > 0 and number(addon, "PrevQuestID") in self.disabled:
            findings.add("previous_quest_disabled")
        relations = []
        for relation in self.relations.get(quest, []):
            kind, entry = relation["kind"], number(relation, "id")
            entry_kind = kind.removeprefix("event_").split("_")[0]
            actor = self.actor(entry_kind, entry) if entry_kind in ("npc", "go") else {"entry": entry}
            relations.append([kind, actor])
            if actor.get("template_present") is False:
                findings.add("giver_or_receiver_template_missing")
            if entry_kind in ("npc", "go") and not (actor["spawn_count"] or actor["summon_references"]):
                findings.add("giver_or_receiver_placement_untraced")
            if kind in ("npc_start", "npc_end") and actor.get("npc_flags", 0) & 2 == 0:
                if not any(number(s, "npcflag") & 2 for s in self.spawns["npc"].get(entry, [])):
                    findings.add("npc_relation_questgiver_flag_untraced")
        result["relations"] = relations
        result["has_placed_offer_actor"] = any(
            kind.endswith("_start") and actor.get("spawn_count", 0) for kind, actor in relations)
        result["seasonal_relations"] = self.event_relations.get(quest, [])
        result["quest_conditions"] = [r for source in (19, 20) for r in self.conditions.get((source, 0, quest, 0), [])]
        result["phases"] = self.phase_quests.get(quest, [])
        result["spell_area"] = self.spell_area.get(quest, [])
        result["object_visuals"] = self.object_visuals.get(quest, [])
        result["poi_count"] = len(self.quest_poi.get(quest, []))
        result["quest_spells"] = self.quest_spells.get(quest, [])
        result["smart_quest"] = self.smart_quest.get(quest, [])
        result["legacy_start"] = self.start_scripts.get(number(row, "StartScript"), [])
        result["legacy_end"] = self.end_scripts.get(number(row, "CompleteScript"), [])
        result["legacy_event_credit"] = self.db_quest_credit.get(quest, [])
        starter_items = [entry for entry, _ in self.item_starters.get(quest, [])]
        result["offer_reference"] = bool(any(kind.endswith("_start") for kind, _ in relations) or starter_items or
                any(r[1] == 150 for r in result["quest_spells"]) or
                any(number(s, "action_type") == 7 for s in result["smart_quest"]))
        if not result["offer_reference"]:
            findings.add("quest_start_route_untraced")
        if not any(kind.endswith("_end") for kind, _ in relations) and not (flags & (0x400 | 0x10000)):
            findings.add("quest_turn_in_route_untraced")
        items = [(item, "starter", 1) for item in starter_items]
        if start := number(row, "StartItem"):
            items.append((start, "provided", number(addon, "ProvidedItemCount") or 1))
        for slot in range(1, 5):
            if drop := number(row, f"ItemDrop{slot}"):
                items.append((drop, "extra_drop", number(row, f"ItemDropQuantity{slot}")))
        objectives = []
        slots = Counter()
        for objective in self.objectives.get(quest, []):
            oid, kind, entry, amount, storage, opt_flags = integers(
                objective, "ID", "Type", "ObjectID", "Amount", "StorageIndex", "Flags")
            detail = {"id": oid, "type": OBJECTIVES[kind] if 0 <= kind < len(OBJECTIVES) else kind,
                      "entry": entry, "amount": amount, "slot": storage, "flags": opt_flags,
                      "description": objective["Description"], "optional": bool(opt_flags & 4)}
            if amount == 0 and kind in (0, 1, 2, 3) and not detail["optional"]:
                findings.add("required_objective_zero_amount")
            if storage >= 0 and kind not in (10, 14):
                slots[storage] += 1
            if kind == 1 and entry:
                items.append((entry, "objective", amount))
            elif kind in (0, 2, 3, 11):
                actor_kind = "go" if kind == 2 else "npc"
                detail["actor"] = actor = self.actor(actor_kind, entry)
                if actor.get("template_present") is False:
                    findings.add("objective_actor_template_missing")
                aliases = self.reverse_credit.get(entry, []) if kind == 0 else []
                detail["credit_aliases"] = [[a, self.spawn_info("npc", a)] for a in aliases]
                detail["credit_effects"] = self.credit.get(entry, []) if kind == 0 else []
                detail["smart_credit"] = self.smart_credit.get(entry, []) if kind in (0, 2) else []
                if not (actor["spawn_count"] or actor["summon_references"] or detail["credit_effects"] or
                        detail["smart_credit"] or any(self.spawns["npc"].get(a) for a in aliases)):
                    findings.add("objective_actor_or_credit_route_untraced")
                if kind == 2:
                    if actor["static_unphased_ungated_count"]:
                        findings.add("static_object_visible_without_template_phase_gate")
                    if actor.get("flags", 0) & 0x16:
                        findings.add("object_interaction_requires_unlock_or_flag_review")
                    if actor.get("quest_gate") in self.quests and actor["quest_gate"] != quest:
                        findings.add("object_template_points_to_different_quest")
                    if actor.get("display") and actor["display"] not in self.displays:
                        findings.add("object_display_missing_from_client_db2")
            elif kind == 5 and entry not in self.spells:
                findings.add("objective_spell_missing")
            elif kind == 10:
                detail["native_area_trigger"] = entry in self.area_triggers
                detail["area_scripts"] = self.area_scripts.get(entry, [])
            elif kind == 14 and entry not in self.criteria:
                findings.add("objective_criteria_tree_missing")
            objectives.append(detail)
        if any(count > 1 for count in slots.values()):
            findings.add("objective_storage_slot_shared")
        result["objectives"] = objectives
        result["native_objectives"] = self.native_objectives.get(quest, [])
        if result["native_objectives"]:
            actual = {(o["id"], o["entry"], o["amount"]) for o in objectives}
            if any((r[0], r[3], r[4]) not in actual for r in result["native_objectives"]):
                findings.add("native_objective_differs_from_world_sql")
        if start and any(role == "objective" and item == start for item, role, _ in items):
            findings.add("same_item_provided_and_required_review_cleanup")
        abandon_items = {entry for entry, role, _ in items if role in ("provided", "extra_drop")}
        result["items"] = []
        for item, role, amount in items:
            item_info = self.item(item, quest, role, amount, findings)
            cleanup = {
                "objective": "retained_by_flags_ex" if flags_ex & 1 else "consume_objective_amount",
                "provided": "retained_by_special_flags" if special & 16 else "remove_provided_quantity",
                "extra_drop": ("retained_by_flags_ex_and_special_flags" if flags_ex & 1 and special & 16 else
                               "remove_configured_quantity_or_9999"),
                "starter": "separate_item_starter_and_objective_rules",
            }[role]
            item_info["turn_in_policy"] = cleanup
            if role == "objective":
                item_info["abandonment_has_provided_or_drop_entry"] = item in abandon_items
                if (item not in abandon_items and item_info["class"] == 12
                        and any(route[5] for route in item_info["loot_routes"])):
                    findings.add("quest_only_loot_abandonment_cleanup_requires_review")
            result["items"].append(item_info)
        result["findings"] = sorted(findings)
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exports", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--release", type=Path, default=ROOT / "build-extractors/bin/Release")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    evidence = Evidence(args.exports, args.release / "dbc/enUS", args.release / "worldserver.exe")
    audit = Audit(evidence)
    all_records = [audit.quest(q) for q in sorted(audit.scope)]
    records = []
    without_offer = []
    for record in all_records:
        quest = record["id"]
        if quest in audit.primary_scope:
            record["scope"] = "level_or_minimum"
            records.append(record)
        elif record["offer_reference"]:
            record["scope"] = "zero_minimum_with_offer_reference"
            records.append(record)
        else:
            without_offer.append({key: record[key] for key in ("id", "title", "level", "disabled")})
    cpp, names = cpp_evidence(audit.scope, audit.cpp_names, evidence.binary)
    results = Counter()
    findings_with_placed_offer = Counter()
    types = Counter()
    evidence_sets = Counter()
    with (args.output / "quests.jsonl").open("w", encoding="utf-8") as stream:
        for record in records:
            record["cpp_quest_id_candidates"] = cpp.get(record["id"], [])
            if record["disabled"]:
                evidence_sets["disabled"] += 1
            else:
                results.update(record["findings"])
                if record["has_placed_offer_actor"]:
                    findings_with_placed_offer.update(record["findings"])
                types.update(o["type"] for o in record["objectives"])
            evidence_sets["authored_level_1_20"] += 1 <= record["level"] <= 20
            evidence_sets["minimum_level_1_20"] += 1 <= record["minimum"] <= 20
            evidence_sets["scaling_level_minus_one"] += record["level"] == -1
            evidence_sets["zero_minimum_with_offer_reference"] += record["scope"] == "zero_minimum_with_offer_reference"
            evidence_sets["enabled_with_item"] += bool(record["items"]) and not record["disabled"]
            evidence_sets["enabled_with_on_use_item"] += any(i.get("uses") for i in record["items"]) and not record["disabled"]
            evidence_sets["enabled_cpp_quest_id_candidate"] += bool(record["cpp_quest_id_candidates"]) and not record["disabled"]
            stream.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n")
    with (args.output / "candidates.tsv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(("quest", "title", "quest_level", "min_level", "disabled", "sort", "findings"))
        for r in records:
            writer.writerow((r["id"], r["title"], r["level"], r["minimum"], r["disabled"],
                             r["sort"], ", ".join(r["findings"])))
    for path in sorted(args.exports.glob("*.tsv")):
        evidence.hashes.setdefault(f"sql/{path.name}", file_hash(path))
    summary = {"scope": "1 <= QuestLevel <= 20 OR 1 <= MinLevel <= 20; MinLevel=0 with a traced offer reference",
               "database_quests": len(audit.quests), "scoped": len(records),
               "enabled": len(records) - evidence_sets["disabled"], "coverage": evidence_sets,
               "zero_minimum_without_offer_reference": len(without_offer),
               "zero_minimum_without_offer_with_cpp_quest_id_candidate": sum(bool(cpp.get(r["id"])) for r in without_offer),
               "enabled_objective_types": types, "enabled_quest_candidate_counts": results,
               "enabled_candidate_counts_with_placed_offer": findings_with_placed_offer,
               "script_names": len(names),
               "script_names_with_release_string": sum(r["name_string_in_release"] for r in names.values()),
               "script_names_without_release_string": [k for k, r in names.items() if not r["name_string_in_release"]],
               "worldserver_sha256": file_hash(evidence.binary), "input_sha256": evidence.hashes,
               "limits": "Static evidence. A name, source reference, placement, phase or DB2 effect is not a gameplay pass."}
    for record in without_offer:
        record["cpp_quest_id_candidates"] = cpp.get(record["id"], [])
    for name, data in (("summary", summary), ("cpp-bindings", names), ("zero-minimum-without-offer", without_offer)):
        (args.output / (name + ".json")).write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k != "input_sha256"}, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
