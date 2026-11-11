from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/FortySecondTenRepairs.json','bf4b28a50c994062cb1faf3203ea4d1d40058c2f',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','FortySecondNative')
