"""Execute a complete certificate root with one shared read-only PE parse.

Some roots compose the same base reader many times. Only its successful result
is reused within this one process; every certificate body still executes.
"""
import argparse
import importlib.abc
import importlib.util
import json
import linecache
import runpy
import sys
from pathlib import Path

original = runpy.run_path
reader = Path(__file__).with_name('verify_scenario_lfg_release.py').resolve()
cached = None
parser = argparse.ArgumentParser()
parser.add_argument('verifier', type=Path)
parser.add_argument('release', type=Path)
parser.add_argument('--baseline-sources', type=Path)
args = parser.parse_args()
sources = json.loads(args.baseline_sources.read_text(encoding='utf8')) if args.baseline_sources else None
directory = reader.parent

def execute(path, init_globals=None, run_name=None):
    path = Path(path).resolve()
    if sources is None or path.parent != directory:
        return original(str(path), init_globals=init_globals, run_name=run_name)
    assert path.name in sources, 'Pinned certificate dependency absent: ' + path.name
    scope = dict(init_globals or {})
    scope.update(__file__=str(path), __name__=run_name or '<run_path>', __package__='', __spec__=None, __loader__=None, __cached__=None)
    source = sources[path.name]
    linecache.cache[str(path)] = (len(source), None, source.splitlines(True), str(path))
    exec(compile(source, str(path), 'exec'), scope)
    return scope

class PinnedTools(importlib.abc.MetaPathFinder, importlib.abc.Loader):
    def find_spec(self, fullname, path=None, target=None):
        candidate = directory / (fullname + '.py')
        if '.' not in fullname and candidate.is_file():
            assert candidate.name in sources, 'Unpinned local dependency: ' + fullname
            return importlib.util.spec_from_file_location(fullname, candidate, loader=self)
        return None
    def create_module(self, spec):
        return None
    def exec_module(self, module):
        source = sources[Path(module.__file__).name]
        linecache.cache[module.__file__] = (len(source), None, source.splitlines(True), module.__file__)
        exec(compile(source, module.__file__, 'exec'), module.__dict__)

def shared_reader(path, init_globals=None, run_name=None):
    global cached
    if Path(path).resolve() != reader or init_globals is not None:
        return execute(path, init_globals=init_globals, run_name=run_name)
    if cached is None:
        cached = execute(path, run_name=run_name)
    result = dict(cached)
    # Each consumer receives its own maps/lists; immutable binary bytes and the
    # reader's functions refer to the unchanged original parse.
    for key in ('symbols',):
        result[key] = dict(cached[key])
    for key in ('sections', 'functions', 'starts'):
        result[key] = list(cached[key])
    return result

verifier = args.verifier
sys.argv = [str(verifier), str(args.release)]
finder = PinnedTools() if sources is not None else None
if finder:
    sys.meta_path.insert(0, finder)
runpy.run_path = shared_reader
try:
    execute(str(verifier), run_name='__main__')
except SystemExit as error:
    raise RuntimeError(f'Certificate root exited before completing: {verifier}, status={error.code!r}') from error
finally:
    runpy.run_path = original
    if finder:
        sys.meta_path.remove(finder)
print('PASS linked verifier returned through the complete check chain')
