"""Native persistent pipe tests, including actual RGB payloads and multiplier semantics."""
import os
from pathlib import Path
import sys
import tempfile
import math
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from desktop.model import profile
from desktop.runtime import EnginePipe
from PIL import Image

root=Path(__file__).resolve().parents[1]
binary=root/'dist'/('UniFrame.exe' if os.name=='nt' else 'uniframe')
def moving(dx):
    raw=bytearray()
    for y in range(144):
        for x in range(256):
            u=x-dx;c=.5+.19*math.sin(u*.27+y*.13)+.17*math.cos(u*.11-y*.31)+.10*math.sin(u*.49+y*.37)
            raw.extend(round(max(0,min(1,value))*255) for value in [c,c*.8,c*.6])
    return Image.frombytes('RGB',(256,144),bytes(raw))
motion_frames=[moving(dx) for dx in [0,8,16]]
with tempfile.TemporaryDirectory(prefix='eka-stream-') as tmp:
    for backend in ['cpu','vulkan']:
        for mode in ['repeat','blend','interpolate','adaptive']:
            for multiplier in ([2,3,4] if mode in ['repeat','blend'] else [2,3]):
                p=profile();p.update(mode=mode,multiplier=multiplier,backend=backend,blend_weight=.25)
                if mode=='interpolate' and multiplier==3:p['quality']='quality'
                engine=EnginePipe(binary,Path(tmp)/'settings.json',p)
                try:
                    data,images=engine.process_frame(motion_frames[0] if mode in ['interpolate','adaptive'] else Image.new('RGB',(256,144),(80,80,80)),0,0)
                    assert data['payload_frame_count']==1 and data['generated_count']==0
                    data,images=engine.process_frame(motion_frames[1] if mode in ['interpolate','adaptive'] else Image.new('RGB',(256,144),(100,100,100)),1,100)
                    assert data['real_game_fps'] is None and data['displayed_fps'] is None and data['input_latency_ms'] is None
                    if mode=='repeat':
                        assert len(images)==1 and images[0].getpixel((0,0))==(100,100,100)
                        assert data['repeated_count']==multiplier-1 and data['generated_count']==0 and len(data['kinds'])==multiplier
                    elif mode=='blend':
                        assert len(images)==multiplier and data['generated_count']==multiplier-1
                        assert images[1].getpixel((0,0))==(85,85,85)
                    else:
                        assert len(images)==multiplier and data['generated_count']==multiplier-1
                        if multiplier>2:assert images[1].tobytes()!=images[2].tobytes(),'Different interpolation phases must differ'
                    # Repeated calls reuse device/command/fence and do not exhaust descriptor pools.
                    data,_=engine.process_frame(motion_frames[2] if mode in ['interpolate','adaptive'] else Image.new('RGB',(256,144),(110,110,110)),2,200)
                    assert data['id']==2
                    engine.log.seek(0);log=engine.log.read().decode('utf-8','replace')
                    assert 'Validation Error' not in log and 'SYNC-HAZARD' not in log,log
                finally:engine.close()
    p=profile();p.update(mode='interpolate',backend='cpu')
    engine=EnginePipe(binary,Path(tmp)/'settings.json',p)
    try:
        engine.process_frame(Image.new('RGB',(256,144),(80,80,80)),0,0)
        data,_=engine.process_frame(Image.new('RGB',(256,144),(100,100,100)),1,100)
        assert data['generated_count']==0 and data['repeated_count']==1,'Artifact fallback must not count as a new image'
    finally:engine.close()
    p.update(mode='blend',blend_weight=.25)
    engine=EnginePipe(binary,Path(tmp)/'settings.json',p)
    try:
        engine.process_frame(Image.new('RGB',(256,144),(80,80,80)),0,0)
        data,images=engine.process_frame(Image.new('RGB',(256,144),(81,81,81)),1,100)
        assert images[1].getpixel((0,0))==(80,80,80) and data['generated_count']==0,'Sub-code-value changes are not distinct RGB images'
    finally:engine.close()
    print('PASS: persistent CPU/Vulkan RGB streaming, 2x/3x/4x repeat/blend, actual new interpolation images, fallback accounting, resource reuse and null physical metrics.')
