"""Build a desktop distribution on the target OS; PyInstaller cannot cross-build Windows."""
import os
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
native=root/'dist'/('UniFrame.exe' if os.name=='nt' else 'uniframe')
if not native.is_file():raise SystemExit('Build the native processing engine first.')
command=[sys.executable,'-m','PyInstaller','--noconfirm','--clean','--onedir','--windowed',
    '--exclude-module','numpy',
    '--hidden-import','PIL._tkinter_finder','--hidden-import','PIL._imagingtk',
    '--name','UniFrame-Desktop','--distpath',str(root/'dist/desktop'),
    '--workpath',str(root/'build/freeze'),'--specpath',str(root/'build'),
    '--add-data',str(root/'locales')+os.pathsep+'locales',
    '--add-data',str(root/'docs')+os.pathsep+'docs',
    '--add-data',str(root/'third_party/nlohmann/LICENSE.MIT')+os.pathsep+'licenses',
    '--add-binary',str(native)+os.pathsep+'.',str(root/'desktop_entry.py')]
# Some portable Python distributions preload Tcl/Tk from sys.prefix/lib;
# their extension has no RPATH. Include those verified installed libraries.
if sys.platform.startswith('linux'):
    for library in sorted((Path(sys.prefix)/'lib').glob('libtcl*.so')):
        command[-1:-1]=['--add-binary',str(library)+os.pathsep+'.']
environment=dict(os.environ,PYINSTALLER_CONFIG_DIR=str(root/'build/pyinstaller-cache'))
subprocess.run(command,check=True,cwd=root,env=environment)
# Carry the notices for the actual bundled runtime and Python packages.
import importlib.metadata
import shutil
notices=root/'dist/desktop/UniFrame-Desktop/_internal/licenses'
notices.mkdir(parents=True,exist_ok=True)
for name in ('Pillow','psutil'):
    distribution=importlib.metadata.distribution(name)
    for file in distribution.files or []:
        if Path(str(file)).name.lower().startswith('license'):
            source=Path(distribution.locate_file(file))
            if source.is_file():shutil.copy2(source,notices/(name+'-'+source.name))
python_license=Path(sys.prefix)/f'lib/python{sys.version_info.major}.{sys.version_info.minor}/LICENSE.txt'
if python_license.is_file():shutil.copy2(python_license,notices/'Python-LICENSE.txt')
for source in (root/'third_party/runtime-notices').glob('*'):
    if source.is_file():shutil.copy2(source,notices/source.name)
