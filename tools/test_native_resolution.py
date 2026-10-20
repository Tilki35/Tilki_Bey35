"""No downscaling: exact Full HD source pixels and odd-size CPU/Vulkan synthesis."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from PIL import Image,ImageChops,ImageStat
from desktop.capture import prepare
from desktop.model import profile
from desktop.runtime import EnginePipe
root=Path(__file__).resolve().parents[1]
records=[]
with tempfile.TemporaryDirectory(prefix='eka-native-size-') as tmp:
    for backend in ['cpu','vulkan']:
        size=(1920,1080)
        # Single-pixel detail must survive byte for byte; it cannot survive a
        # hidden downscale/upscale roundtrip.
        raw=bytes((i*37+i//1920)%256 for i in range(size[0]*size[1]*3))
        a=Image.frombytes('RGB',size,raw);assert prepare(a) is a
        b=ImageChops.invert(a)
        for mode in ['repeat','blend','interpolate']:
            b=ImageChops.offset(a,2,0) if mode=='interpolate' else ImageChops.invert(a)
            p=profile();p.update(mode=mode,backend=backend,blend_weight=.5,multiplier=2,quality='eco')
            engine=EnginePipe(root/'dist/uniframe',Path(tmp)/'settings.json',p,size)
            try:
                first,images=engine.process_frame(a,0,0)
                assert images[0].tobytes()==raw and images[0].size==size
                start=time.perf_counter();data,images=engine.process_frame(b,1,16.667)
                assert (data['width'],data['height'])==size
                if mode=='repeat':
                    assert images[0].tobytes()==b.tobytes() and data['generated_count']==0 and data['repeated_count']==1
                elif mode=='blend':
                    assert images[0].tobytes()==raw and images[1].size==size
                    assert images[1].getextrema()==((128,128),(128,128),(128,128))
                    assert data['generated_count']==0 and data['blended_count']==1 and data['lookahead_ms']==16.667
                else:
                    assert images[1].size==size and data['generated_count']==1
                    assert images[1].tobytes() not in [a.tobytes(),b.tobytes()]
                    assert data['lookahead_ms']==16.667
                records.append({'requested_backend':backend,'actual_backend':data['backend'],'mode':mode,'size':size,'processing_ms':data['processing_ms'],'pipe_roundtrip_ms':(time.perf_counter()-start)*1000})
                engine.log.seek(0);log=engine.log.read().decode('utf-8','replace')
                assert 'Validation Error' not in log and 'SYNC-HAZARD' not in log,log
            finally:engine.close()
        size=(321,181)
        raw=bytes((x*13+y*7+(x//9)*19)%256 for y in range(size[1]) for x in range(size[0]) for _ in range(3))
        a=Image.frombytes('RGB',size,raw);b=ImageChops.offset(a,2,0)
        p=profile();p.update(mode='interpolate',backend=backend,quality='eco')
        engine=EnginePipe(root/'dist/uniframe',Path(tmp)/'settings.json',p,size)
        try:
            engine.process_frame(a,0,0);data,images=engine.process_frame(b,1,16.667)
            assert data['generated_count']==1 and images[1].size==size
            assert images[1].tobytes() not in [a.tobytes(),b.tobytes()]
            if backend=='cpu':reference=images[1]
            else:assert max(ImageStat.Stat(ImageChops.difference(reference,images[1])).mean)<.1
        finally:engine.close()
    for args in [['--width','0'],['--width','4097'],['--width','4096','--height','4096'],['--width','3840','--height','2160','--multiplier','8']]:
        result=subprocess.run([str(root/'dist/uniframe'),'--stream','--backend','cpu',*args],input=b'',capture_output=True,timeout=5)
        assert result.returncode!=0,args
out=root/'results/native-resolution';out.mkdir(parents=True,exist_ok=True)
(out/'measurements.json').write_text(json.dumps(records,indent=2))
print('PASS: exact 1920x1080 repeat and blended output, odd-size real interpolation CPU/Vulkan parity, no resizing, bounded geometry and explicit lookahead.')
