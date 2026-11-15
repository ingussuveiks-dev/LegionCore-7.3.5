"""Reuse native loader/loot/quest fixture generation for the six-entry plan.

The fishing pool retains native ranged LOOT_FISHINGHOLE admission. Ordinary
LOOT_CORPSE still supplies the common real20yd distance-denial control.
"""
import runpy,sys
from pathlib import Path
import layered_ten_ten as shared
from tail_loot import load,filename
shared.load=load
shared.filename=filename
runpy.run_path(str(shared.ROOT/'tools/tests/prepare_layered_native.py'),run_name='__main__')
out=Path(sys.argv[2]);f=out/'LayeredNative.cpp';s=f.read_text(encoding='utf-8')
s=s.replace('PASS63','PASS6').replace('29 actual quest/objective','4 actual quest/objective')
s=s.replace('info&&info->type==GAMEOBJECT_TYPE_CHEST','info&&info->type==(entry==211423?GAMEOBJECT_TYPE_FISHINGHOLE:GAMEOBJECT_TYPE_CHEST)')
s=s.replace('owner.SendLoot(source.GetGUID(),LOOT_CORPSE);','owner.SendLoot(source.GetGUID(),entry==211423?LOOT_FISHINGHOLE:LOOT_CORPSE);')
s=s.replace('     auto gate=Gate(entry);', '''     if(entry==211423){auto gate=Gate(entry);Require(gate,"Fishing quest gate missing");owner.SetQuestSlot(0,gate->quest,0);owner.SetQuestStatus(gate->quest,QUEST_STATUS_INCOMPLETE);Chest pool(map,entry,guid++,pos);owner.Relocate(pos.GetPositionX()+20.f,pos.GetPositionY(),pos.GetPositionZ(),0);owner.SendLoot(pool.GetGUID(),LOOT_FISHINGHOLE);Require((Earned(owner,pool)->GetLootOwner()==&owner)==repaired,"Native ranged fishing-hole source generation differs");owner.Relocate(pos);owner.SetLootGUID(ObjectGuid::Empty);owner.personalLoot.erase(pool.GetGUID());owner.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_LOOTING);owner.SetQuestStatus(gate->quest,QUEST_STATUS_NONE);owner.SetQuestSlot(0,0,0);}
     auto gate=Gate(entry);''')
s=s.replace('2 factions/distance denials','2 factions/distance denials and native ranged fishing-hole admission')
f.write_text(s,encoding='utf-8')
