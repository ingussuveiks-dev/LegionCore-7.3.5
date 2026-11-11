from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/FortyThirdTenRepairs.json','308d97ee2d3a966936bf5c7621e0eee91b6516d7',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','FortyThirdNative')
