from pathlib import Path
import sys
from prepare_death_owner import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/FortySixthTenRepairs.json','ca5c31f164c11fe90470098b7df6b42b4e568969',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline')
