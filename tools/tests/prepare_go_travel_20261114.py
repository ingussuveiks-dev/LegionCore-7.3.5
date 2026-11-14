"""Export full current travel handlers and native coordinate overloads."""
from pathlib import Path
import json,re,sys
from prepare_five_ten_20261114 import ROOT,PLAN,verify,block

def coordinates(out):
 source=(ROOT/'src/server/game/Grids/GridDefines.h').read_text()
 names=('MAX_NUMBER_OF_GRIDS','SIZE_OF_GRIDS','CENTER_GRID_ID','MAP_SIZE','MAP_HALFSIZE')
 macros='\n'.join(re.search(r'^#define '+name+r'\s+[^\n]+',source,re.M)[0]for name in names)
 bodies='\n'.join(block(source,'inline bool IsValidMapCoord('+signature+')')for signature in ('float c','float x, float y','float x, float y, float z','float x, float y, float z, float o'))
 (out/'NativeCoordinates.inc').write_text(macros+'\n#define finite__ std::isfinite\nnamespace Trinity {\n'+bodies+'\n}\n#undef finite__\n')

def prepare(out,baseline=False,only_coordinates=False):
 verify();out.mkdir(parents=True,exist_ok=True);coordinates(out)
 if only_coordinates:return
 selected=[];defines=[]
 for row in PLAN['changes']:
  if row['kind']!='gotravel' or baseline and row['case']!='GoObject':continue
  source=(ROOT/row['path']).read_text();code=row['old']if baseline else block(source,'    static bool '+row['method']+'(')
  selected.append('namespace Case'+row['case']+'{struct Selected{'+code+'};}')
  defines.append('GO_OBJECT_BASELINE'if baseline else 'REPAIR_'+row['case'].upper())
 (out/'TravelSelected.inc').write_text('\n'.join(selected))
 headers=(ROOT/'src/server/game/Miscellaneous/Language.h').read_text();joined='\n'.join(selected)
 constants=[]
 for name in sorted(set(re.findall(r'\bLANG_[A-Z_0-9]+\b',joined))):
  match=re.search(r'\b'+name+r'\s*=\s*(\d+)\b',headers);assert match,name;constants.append('constexpr uint32 '+name+'='+match[1]+';')
 for name,relative in [('MAX_HEIGHT','src/server/game/Maps/Map.h'),('INVALID_HEIGHT','src/server/game/Maps/Map.h')]:
  value=re.search(r'^#define '+name+r'\s+([^\s]+)',(ROOT/relative).read_text(),re.M)[1];constants.append('constexpr float '+name+'='+value+';')
 dbc=(ROOT/'src/server/game/DataStores/DBCEnums.h').read_text()
 for name in ('DIFFICULTY_N_SCENARIO','DIFFICULTY_HC_SCENARIO'):constants.append('constexpr uint32 '+name+'='+re.search(name+r'\s*=\s*(\d+)',dbc)[1]+';')
 (out/'TravelConstants.inc').write_text('\n'.join(constants))
 (out/'defines.txt').write_text(' '.join('/D'+name for name in defines))
 (out/'source-inputs.json').write_text(json.dumps({'files':PLAN['files'],'old_object_counterfactual':baseline,'scope':'Full actual travel handlers/native coordinate overloads;world/native stores,terrain,SQL,movement/teleport are controlled,no live changes'},indent=2))
 print('PASS actual full selected travel source and native coordinate overloads;baseline='+str(baseline))
if __name__=='__main__':prepare(Path(sys.argv[1]),len(sys.argv)>2 and sys.argv[2]=='baseline',len(sys.argv)>2 and sys.argv[2]=='coordinates-only')
