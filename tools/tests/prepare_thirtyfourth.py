from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/ThirtyFourthTenRepairs.json','8c7da6ef842a755dba2905fc9ed7696b1fa5d47f',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','ThirtyFourthNative')
