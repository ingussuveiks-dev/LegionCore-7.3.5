"""Run each retained artifact PE root to completion and preserve its diagnostics.

This verifies declared binary paths. It does not certify whole client chains.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('release', type=Path)
parser.add_argument('--out', required=True, type=Path)
parser.add_argument('--counterfactual', action='store_true')
parser.add_argument('--roots-json', type=Path)
parser.add_argument('--baseline-ref')
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
tools = repo / 'tools/tests'
catalog = args.roots_json or repo / 'docs/audits/ten-ten-2026-11-11/LegacyArtifactPeOpen.json'
data = json.loads(catalog.read_text(encoding='utf-8-sig'))
roots = data['roots'] if isinstance(data, dict) else data
assert roots and len(roots) == len(set(roots))
assert all(Path(name).name == name and name.startswith('verify_') and name.endswith('.py') for name in roots)
args.out = args.out.resolve()
assert args.out.is_relative_to(repo), 'Certificate evidence must remain inside this workspace'
args.out.mkdir(parents=True, exist_ok=True)
release = args.release.resolve()
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
before = {name: sha(release / name) for name in ['worldserver.exe', 'worldserver.map']}
source_hashes = {path.name: sha(path) for path in tools.iterdir() if path.suffix in ('.py', '.ps1')}
baseline_sources = None
if args.baseline_ref:
    assert args.counterfactual, 'Pinned historical tools are a failing control, not an ordinary certificate'
    baseline = subprocess.check_output(['git', '-C', str(repo), 'rev-parse', args.baseline_ref + '^{commit}'], text=True).strip()
    paths = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-r', '--name-only', baseline, 'tools/tests'], text=True).splitlines()
    paths = [path for path in paths if path.endswith('.py')]
    raw = subprocess.check_output(['git', '-C', str(repo), 'cat-file', '--batch'], input=''.join(baseline + ':' + path + '\n' for path in paths).encode())
    stream = io.BytesIO(raw)
    pinned = {}
    for path in paths:
        header = stream.readline().split()
        assert len(header) == 3 and header[1] == b'blob'
        pinned[Path(path).name] = stream.read(int(header[2])).decode('utf-8-sig', errors='surrogateescape')
        assert stream.read(1) == b'\n'
    assert not stream.read()
    baseline_sources = args.out / 'baseline-sources.json'
    baseline_sources.write_text(json.dumps(pinned), encoding='utf8')
    print('PINNED TOOL COUNTERFACTUAL ' + baseline + ' on unchanged current binary; this is not a production gameplay baseline.', flush=True)
env = dict(os.environ, PYTHONUTF8='1')
env['PYTHONPATH'] = str(repo / '.codex/pe-review-deps') + os.pathsep + str(tools) + os.pathsep + env.get('PYTHONPATH', '')

def run(name):
    output = args.out / (name + '.log')
    with output.open('wb') as stream:
        command = [sys.executable, str(tools / 'run_artifact_certificate_root.py'), str(tools / name), str(release)]
        if baseline_sources:
            command += ['--baseline-sources', str(baseline_sources)]
        process = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, env=env, cwd=repo)
    text = output.read_text(encoding='utf8', errors='replace')
    completed = process.returncode == 0 and 'PASS linked verifier returned through the complete check chain' in text
    return {'root': name, 'exit': process.returncode, 'complete': completed, 'log': str(output.relative_to(repo)), 'log_sha256': sha(output)}

results = []
with ThreadPoolExecutor(max_workers=3) as executor:
    for result in map(lambda future: future.result(), as_completed([executor.submit(run, name) for name in roots])):
        results.append(result)
        print(('PASS' if result['complete'] else 'OPEN') + ' ' + result['root'] + ' exit=' + str(result['exit']), flush=True)
assert before == {name: sha(release / name) for name in before}, 'Inspected binary or map changed'
assert source_hashes == {path.name: sha(path) for path in tools.iterdir() if path.suffix in ('.py', '.ps1')}, 'Certificate sources changed during collection; mixed results cannot certify this run'
results.sort(key=lambda row: row['root'])
failed = [row for row in results if not row['complete']]
report = {'status': 'EXPECTED_BROKEN_COUNTERFACTUAL' if args.counterfactual else 'PASS_DECLARED_BINARY_SCOPES' if not failed else 'OPEN_FAILED_CERTIFICATES', 'artifacts': before, 'roots': results, 'passed': len(results) - len(failed), 'failed': len(failed), 'limitations': 'Actual client and whole-chain behavior remain separate; no failed root is silently skipped.'}
report['current_tool_hashes'] = source_hashes
if baseline_sources:
    report['tool_counterfactual_commit'] = baseline
    report['pinned_sources_sha256'] = sha(baseline_sources)
(args.out / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf8')
if args.counterfactual:
    assert failed, 'Expected pinned failing baseline was not reproduced'
    print(f'EXPECTED BROKEN certificate counterfactual: {len(failed)}/{len(results)} fail; repaired ordinary run required.')
else:
    print(f'Certificate roots: {len(results)-len(failed)}/{len(results)} complete; {len(failed)} OPEN.')
    sys.exit(1 if failed else 0)
