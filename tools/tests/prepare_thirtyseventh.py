from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/ThirtySeventhTenRepairs.json','70f6ff70f35ebac97f1804a0d5c2c7b0c8fd5a68',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','ThirtySeventhNative')
