from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/FortyFifthTenRepairs.json','2ea67fbc7e34c0fc76ef78d5ec217ce73ea16624',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','FortyFifthNative')
