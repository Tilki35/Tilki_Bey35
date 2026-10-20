"""Run under Xvfb: real X11 pixels -> native process -> preview callbacks, plus GUI/model tests."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import tkinter as tk
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from desktop.app import App
from desktop.capture import X11Capture,windows,prepare
from desktop.model import Profiles,effective_processing
from PIL import Image

root_dir=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='eka-desktop-') as tmp:
    directory=Path(tmp)
    model=Profiles(directory/'profiles.json');model.add('Örnek Oyun');model.current.update(mode='repeat',multiplier=4,target_title='Game');model.save()
    copy=model.add('Kopya',True);assert copy['mode']=='repeat' and copy['target_title']=='Game'
    model.remove();assert model.current['id']=='default'
    assert len(Profiles(directory/'profiles.json').data['profiles'])==2
    model.current['power']='eco';assert effective_processing(model.current)['fps']<=5
    model.save();original=(directory/'profiles.json').read_bytes();(directory/'bad.json').write_text('{invalid')
    try:Profiles(directory/'bad.json');raise AssertionError('Corrupt profiles accepted')
    except ValueError:pass
    assert (directory/'bad.json').read_text()=='{invalid'
    # Preserve aspect ratio without stretching to the processing dimensions.
    letterboxed=prepare(Image.new('RGB',(100,100),'red'))
    assert letterboxed.size==(256,144) and letterboxed.getpixel((0,0))==(0,0,0) and letterboxed.getpixel((128,72))==(255,0,0)
    subprocess.run([str(root_dir/'dist/uniframe'),'--config',str(directory/'settings.json'),
                    '--name','EVRKare','--ui-language','tr','--settings'],check=True,capture_output=True)
    root=tk.Tk();source=tk.Toplevel(root);source.title('EKA live test target');source.geometry('256x144+0+0')
    canvas=tk.Canvas(source,bg='#1268ac',highlightthickness=0);canvas.pack(fill='both',expand=True)
    rectangle=canvas.create_rectangle(20,30,70,80,fill='#f4cc32',outline='')
    root.update();source.lift();root.update()
    target=next(item for item in windows() if item.title=='EKA live test target')
    capture=X11Capture(target.id)
    try:
        image=capture.grab();assert image.getpixel((120,110))==(18,104,172),image.getpixel((120,110))
    finally:capture.close()
    app=App(root,directory);root.geometry('1160x840+320+40');root.update()
    errors=[];app.error=lambda error:errors.append(str(error))
    app.profiles.current.update(target_title='EKA live test target',mode='repeat',multiplier=3,backend='cpu',fps_limit=10)
    app.profiles.save();app.advanced.set(True);app.change_view();root.update()
    assert len(app.cards)>=7
    for page in ['performance','guide','world','settings','home']:app.navigate(page);root.update()
    app.profiles.data['theme']='light';app.rebuild();root.update();app.profiles.data['theme']='dark';app.rebuild();root.update()
    app.toggle();assert app.session is not None
    root.update();app.toggle_fullscreen();root.update();assert app.fullscreen
    app.leave_fullscreen();root.update();assert not app.fullscreen
    app.preview.geometry('960x600');root.update()
    begin=time.monotonic();deadline=begin+6;index=0
    while time.monotonic()<deadline:
        index+=1;canvas.coords(rectangle,20+index%100,30,70+index%100,80);source.lift();root.update();time.sleep(.01)
        if time.monotonic()-begin>=2.3 and len(app.metrics.capture_times)>=8 and app.metrics.repeated>=4:break
    root.update();data=app.metrics.snapshot();assert not errors,errors
    assert len(app.metrics.capture_times)>=3 and data['preview_submitted']>=3 and data['repeated_submitted']>=1,data
    assert data['generated_completed']==0 and data['real_game_fps'] is None and data['power_watts'] is None
    assert app.preview_image.width()>512 and app.preview_image.height()>288
    app.sample();root.update();data=app.metrics.snapshot()
    assert data['cpu_core_percent'] is not None and data['process_tree_rss_sum_bytes']>0,data
    destination=root_dir/'results/desktop';destination.mkdir(parents=True,exist_ok=True)
    (destination/'measured-session.json').write_text(json.dumps(data,indent=2))
    app.stop();root.update();assert app.session is None and app.stopping.thread.is_alive() is False
    app.navigate('home');root.update()
    own=next(item for item in windows() if item.title=='EVRKare');capture=X11Capture(own.id)
    try:capture.grab().save(destination/'desktop.png')
    finally:capture.close()
    app.close()
    assert (directory/'desktop.json').is_file()
print('PASS: profiles, corrupt-file preservation, themes/pages, real X11 pixel capture, native stream, preview repetition, stop cleanup and process metrics.')
