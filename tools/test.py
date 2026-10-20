"""Integration tests: both real Vulkan dispatch and independent CPU reference required."""
import json
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
binary = root / 'dist' / ('tilki-fg.exe' if os.name == 'nt' else 'tilki-fg')
results = root / 'results' / 'integration'
results.mkdir(parents=True, exist_ok=True)

def run(backend, name, env=None, expected=0):
    dest = results / name
    dest.mkdir(exist_ok=True)
    # Remove stale results so previous success cannot masquerade as a new run.
    (dest / 'metrics.json').unlink(missing_ok=True)
    completed = subprocess.run([str(binary), '--config', str(results/'settings.json'), '--locale', 'en-US',
                               '--ui-language', 'en', '--backend', backend, '--output', str(dest)],
                               env=env, capture_output=True, text=True, timeout=120)
    print(completed.stdout, end='')
    if completed.stderr:
        print(completed.stderr, end='')
    if completed.returncode != expected:
        raise AssertionError(f'{name}: exit={completed.returncode}, expected={expected}')
    if expected == 0 and ('Validation Error' in completed.stderr or 'SYNC-HAZARD' in completed.stderr):
        raise AssertionError(f'{name}: Vulkan validation reported an error')
    if expected:
        return None
    report = json.loads((dest / 'metrics.json').read_text())
    assert report['passed'] is True
    assert len(report['cases']) == 7
    assert all(c['passed'] for c in report['cases'])
    assert report['real_fps'] is None and report['displayed_fps'] is None
    assert report['input_latency_ms'] is None
    return report

cpu = run('cpu', 'cpu')
gpu = run('vulkan', 'vulkan')
assert all(c['backend'] == 'cpu' for c in cpu['cases'])
assert all(c['backend'] == 'vulkan' for c in gpu['cases'])
assert (results/'cpu/generated.ppm').read_bytes() == (results/'vulkan/generated.ppm').read_bytes()
for case in cpu['cases']:
    name = case['name'] + '.generated.ppm'
    assert (results/'cpu'/name).read_bytes() == (results/'vulkan'/name).read_bytes(), name
missing = dict(os.environ, VK_DRIVER_FILES=str(results/'nonexistent-driver.json'))
fallback = run('auto', 'fallback', missing)
assert all(c['backend'] == 'cpu' for c in fallback['cases'])
run('vulkan', 'missing-driver', missing, expected=1)
invalid = subprocess.run([str(binary), '--backend', 'invalid'], capture_output=True, timeout=10)
assert invalid.returncode == 1
print('PASS: 21 scene checks, all-scene CPU/Vulkan image parity, automatic fallback, strict Vulkan failure, invalid CLI.')
