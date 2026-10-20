"""Launch the actual frozen Linux app without the build Python's environment, then close via WM protocol."""
import ctypes as C
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import tkinter as tk
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from desktop.capture import X11Capture,windows
from desktop.model import Profiles

root=Path(__file__).resolve().parents[1]
binary=root/'dist/desktop/UniFrame-Desktop/UniFrame-Desktop'
class MessageData(C.Union):_fields_=[('b',C.c_char*20),('s',C.c_short*10),('l',C.c_long*5)]
class ClientMessage(C.Structure):
    _fields_=[('type',C.c_int),('serial',C.c_ulong),('send_event',C.c_int),('display',C.c_void_p),
              ('window',C.c_ulong),('message_type',C.c_ulong),('format',C.c_int),('data',MessageData)]
class Event(C.Union):_fields_=[('client',ClientMessage),('padding',C.c_long*24)]
with tempfile.TemporaryDirectory(prefix='eka-frozen-') as tmp, tempfile.TemporaryFile() as log:
    source=tk.Tk();source.title('EKA frozen capture target');source.configure(bg='#248ea6');source.geometry('256x144+1400+0');source.update()
    model=Profiles(Path(tmp)/'uniframe/desktop.json');model.current.update(target_title='EKA frozen capture target',mode='repeat',multiplier=3,backend='cpu');model.save()
    env=dict(os.environ,XDG_CONFIG_HOME=tmp,LC_ALL='C',LANGUAGE='')
    for key in ['PYTHONPATH','PYTHONHOME','LD_LIBRARY_PATH','TCL_LIBRARY','TK_LIBRARY']:env.pop(key,None)
    process=subprocess.Popen([str(binary)],env=env,stdout=log,stderr=subprocess.STDOUT)
    try:
        deadline=time.monotonic()+30;window=None
        while time.monotonic()<deadline and process.poll() is None:
            window=next((item for item in windows() if item.title=='XFrame'),None)
            if window:break
            time.sleep(.1)
        if not window:
            log.seek(0);raise AssertionError(log.read().decode('utf-8','replace'))
        # A mapped top-level precedes Tk's idle layout; wait for initial layout
        # before sending input rather than clicking a partially built window.
        until=time.monotonic()+1
        while time.monotonic()<until:source.update();time.sleep(.02)
        capture=X11Capture(window.id)
        try:
            image=capture.grab();assert image.width>=780 and image.height>=620
            destination=root/'results/desktop';destination.mkdir(parents=True,exist_ok=True);image.save(destination/'frozen.png')
            # Click only our app's Start button, on an isolated Xvfb display.
            # Verify real capture reaches the frozen preview, not just window startup.
            from desktop.capture import XAttributes
            attrs=XAttributes();assert capture.x.XGetWindowAttributes(capture.display,window.id,C.byref(attrs))
            xtst=C.CDLL('libXtst.so.6')
            xtst.XTestFakeMotionEvent.argtypes=[C.c_void_p,C.c_int,C.c_int,C.c_int,C.c_ulong]
            xtst.XTestFakeButtonEvent.argtypes=[C.c_void_p,C.c_uint,C.c_int,C.c_ulong]
            xtst.XTestFakeMotionEvent(capture.display,0,attrs.x+image.width-100,attrs.y+44,0)
            xtst.XTestFakeButtonEvent(capture.display,1,True,0);xtst.XTestFakeButtonEvent(capture.display,1,False,0);capture.x.XSync(capture.display,False)
            preview=None;deadline=time.monotonic()+15
            while time.monotonic()<deadline and process.poll() is None:
                source.update();preview=next((item for item in windows() if 'Processed image' in item.title),None)
                if preview:
                    view=X11Capture(preview.id)
                    try:
                        pixels=view.grab()
                        if pixels.getpixel((pixels.width//2,pixels.height*2//3))==(36,142,166):break
                    finally:view.close()
                time.sleep(.05)
            else:
                log.seek(0);diagnostic=log.read().decode('utf-8','replace')
                capture.grab().save(destination/'frozen-failed.png')
                raise AssertionError('Frozen capture did not reach the preview: '+str([item.title for item in windows()])+'\n'+diagnostic)
            event=Event();event.client.type=33;event.client.send_event=1;event.client.display=capture.display;event.client.window=window.id
            event.client.message_type=capture.x.XInternAtom(capture.display,b'WM_PROTOCOLS',False);event.client.format=32
            event.client.data.l[0]=capture.x.XInternAtom(capture.display,b'WM_DELETE_WINDOW',False)
            capture.x.XSendEvent.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_long,C.POINTER(Event)]
            assert capture.x.XSendEvent(capture.display,window.id,False,0,C.byref(event))
            capture.x.XSync(capture.display,False)
        finally:capture.close()
        assert process.wait(timeout=15)==0
        assert (Path(tmp)/'uniframe/desktop.json').is_file()
    finally:
        if process.poll() is None:process.kill();process.wait(timeout=5)
        source.destroy()
print('PASS: frozen Linux desktop, isolated runtime, actual Start click, real X11 capture-to-preview through bundled native engine, and normal shutdown.')
