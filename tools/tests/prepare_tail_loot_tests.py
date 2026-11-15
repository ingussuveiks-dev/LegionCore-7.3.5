"""Reuse the full-row TEMP regression engine with an explicit separate plan."""
import prepare_layered_tests as tests
from tail_loot import load,filename
tests.load=load
tests.filename=filename
if __name__=='__main__':tests.main()
