from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/ThirtyFifthTenRepairs.json','51fde19dd7f6f6b6d11d775eaed69d5c62ad9643',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','ThirtyFifthNative')
