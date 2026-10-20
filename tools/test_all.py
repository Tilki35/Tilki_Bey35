"""Required native readiness checks. Windows binaries are built but require a Windows host to run."""
import os
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
suffix='.exe' if os.name=='nt' else ''
for target in ('core-tests','language-tests'):
    subprocess.run([str(root/'build'/(target+suffix))],check=True,cwd=root)
for script in ('test.py','test_modes.py','test_localization.py','test_stream.py','test_native_resolution.py'):
    subprocess.run([sys.executable,str(root/'tools'/script)],check=True,cwd=root)
print('PASS: all native test groups completed.')
