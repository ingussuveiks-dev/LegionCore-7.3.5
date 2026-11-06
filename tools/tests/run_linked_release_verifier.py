"""Require a composed PE verifier to finish instead of silently exiting early."""
import runpy
import sys

verifier = sys.argv[1]
sys.argv = sys.argv[1:]
try:
    runpy.run_path(verifier, run_name='__main__')
except SystemExit as error:
    raise RuntimeError(f'Linked verifier exited before returning to its caller: {verifier}, status={error.code!r}') from error
print('PASS linked verifier returned through the complete check chain')
