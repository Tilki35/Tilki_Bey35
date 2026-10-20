"""Package both platforms, editable locales and notices; does not publish a release."""
import hashlib
import shutil
import tarfile
from pathlib import Path
import zipfile

root=Path(__file__).resolve().parents[1]
dest=root/'dist'
assets=[(root/'README.md','README.md')]
assets += [(p,str(p.relative_to(root))) for p in sorted((root/'docs').glob('*.md'))]
assets += [(p,str(p.relative_to(root))) for p in sorted((root/'locales').rglob('*')) if p.is_file()]
assets += [(root/'third_party/nlohmann/LICENSE.MIT','licenses/nlohmann-json-LICENSE.MIT'),
           (root/'third_party/sources.json','licenses/sources.json')]
assets += [(p,str(p.relative_to(root))) for p in sorted((root/'desktop').glob('*.py'))]
assets += [(root/name,name) for name in ('requirements-desktop.txt','requirements-packaging.txt','Install-desktop.cmd','run-desktop.sh')]
artifacts=[]
windows=dest/'UniFrame.exe'
if not windows.is_file():raise SystemExit('Build Windows first: python3 tools/build.py --windows')
archive=dest/'UniFrame-prototype-windows-x64.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as out:
    for path,name in [(windows,'UniFrame.exe'),(dest/'EVRKare.exe','EVRKare.exe'),*assets]:out.write(path,'UniFrame/'+name)
    out.writestr('UniFrame/CALISTIR.txt',
        'UniFrame.exe --settings-menu\r\n'
        'UniFrame.exe --mode adaptive --backend auto --output results\r\n'
        'EVRKare / UniFrame / XFrame ayni yazilimdir.\r\n'
        'Pencere yakalama ve ayri onizleme prototipi; oyunun sunumuna kare eklemez.\r\n'
        'Windows ve fiziksel GPU calistirma testi henuz yapilmadi.\r\n')
    out.writestr('UniFrame/DESKTOP.txt','EVRKare.exe is a Python-dependent GUI launcher, not a standalone application. Install Python 3.10+ with Tk and run Install-desktop.cmd first. Windows runtime capture is untested. See docs/desktop.md.\n')
artifacts += [windows,archive]
# Preserve the old download filenames while using the current branding inside.
legacy=dest/'TilkiFG-prototype-windows-x64.zip';shutil.copy2(archive,legacy);artifacts.append(legacy)
linux=dest/'uniframe'
if linux.is_file():
    archive=dest/'UniFrame-prototype-linux-x64.tar.gz'
    with tarfile.open(archive,'w:gz') as out:
        for path,name in [(linux,'uniframe'),*assets]:out.add(path,arcname='UniFrame/'+name)
    artifacts += [linux,archive]
    legacy=dest/'TilkiFG-prototype-linux-x64.tar.gz';shutil.copy2(archive,legacy);artifacts.append(legacy)
for name in ('tilki-fg','tilki-fg.exe'):
    if (dest/name).is_file():artifacts.append(dest/name)
frozen=dest/'desktop/UniFrame-Desktop'
if frozen.is_dir():
    desktop_archive=dest/'UniFrame-Desktop-linux-x64.tar.gz'
    with tarfile.open(desktop_archive,'w:gz') as archive:archive.add(frozen,arcname='UniFrame-Desktop')
    artifacts.append(desktop_archive)
source_archive=dest/'UniFrame-source.zip'
with zipfile.ZipFile(source_archive,'w',zipfile.ZIP_DEFLATED) as archive:
    for folder in ('desktop','docs','locales','shaders','src','tests','third_party','tools'):
        for path in sorted((root/folder).rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts:archive.write(path,'UniFrame-source/'+str(path.relative_to(root)))
    for name in ('README.md','.gitignore','requirements-desktop.txt','requirements-packaging.txt','desktop_entry.py','Install-desktop.cmd','run-desktop.sh'):
        archive.write(root/name,'UniFrame-source/'+name)
artifacts.extend([source_archive,dest/'EVRKare.exe'])
(dest/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in artifacts))
for artifact in artifacts:
    if artifact.name.startswith('UniFrame-'):print(artifact)
