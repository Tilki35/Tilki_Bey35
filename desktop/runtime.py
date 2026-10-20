"""Bounded capture/processing pipeline and measured application counters."""
from collections import deque
import json
from pathlib import Path
import queue
import subprocess
import sys
import tempfile
import threading
import time
from PIL import Image
import psutil
from .capture import capture_class,prepare
from .model import effective_processing

def creation_flags():return {'creationflags':subprocess.CREATE_NO_WINDOW} if sys.platform=='win32' else {}

class EnginePipe:
    def __init__(self,binary,config,p,size=(256,144)):
        settings=effective_processing(p)
        self.size=tuple(size);width,height=self.size
        if not (1<=width<=4096 and 1<=height<=4096 and width*height<=8294400 and width*height*settings['multiplier']<=16777216):raise RuntimeError('desktop.error.capture_size')
        command=[str(binary),'--config',str(config),'--stream','--mode',settings['mode'],'--backend',settings['backend'],
            '--width',str(width),'--height',str(height),'--multiplier',str(settings['multiplier']),'--source-fps',str(settings['fps']),
            '--estimator',settings['estimator'],'--artifact-reduction','on' if p['artifact_reduction'] else 'off',
            '--blend-weight',str(settings['weight']),'--search-radius',str(settings['radius'])]
        self.log=tempfile.TemporaryFile();self.closed=False
        self.process=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.log,**creation_flags())
    def read(self,count):
        result=self.process.stdout.read(count)
        if len(result)!=count:raise RuntimeError('desktop.error.engine_stopped')
        return result
    def process_frame(self,image,identity,timestamp):
        if image.size!=self.size or image.mode!='RGB':raise RuntimeError('desktop.error.protocol')
        self.process.stdin.write((json.dumps({'id':identity,'timestamp_ms':timestamp})+'\n').encode()+image.tobytes());self.process.stdin.flush()
        line=self.process.stdout.readline(16384)
        if not line or not line.endswith(b'\n'):raise RuntimeError('desktop.error.engine_stopped')
        data=json.loads(line)
        if data.get('protocol')!=1 or data.get('width')!=self.size[0] or data.get('height')!=self.size[1] or not 1<=data.get('payload_frame_count',0)<=8:raise RuntimeError('desktop.error.protocol')
        images=[Image.frombytes('RGB',self.size,self.read(self.size[0]*self.size[1]*3)) for _ in range(data['payload_frame_count'])]
        return data,images
    def close(self):
        if self.closed:return
        self.closed=True
        if self.process.poll() is None:
            self.process.terminate()
            try:self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:self.process.kill();self.process.wait(timeout=2)
        for stream in (self.process.stdin,self.process.stdout):
            try:stream.close()
            except (OSError,ValueError):pass
        self.log.close()

class Metrics:
    def __init__(self):
        self.capture_times=deque(maxlen=120);self.submit_times=deque(maxlen=240)
        self.generated=0;self.blended=0;self.repeated=0;self.submitted=0;self.dropped=0;self.capture_ms=None;self.processing_ms=None
        self.pipeline_ms=None;self.lookahead_ms=None;self.width=None;self.height=None;self.backend=None;self.device=None;self.rss=None;self.cpu=None;self.processes={};self.warmed=False
        self.lock=threading.Lock()
    @staticmethod
    def rate(times):return (len(times)-1)/(times[-1]-times[0]) if len(times)>1 and times[-1]>times[0] else None
    def snapshot(self):
        with self.lock:return {'capture_fps':self.rate(self.capture_times),'preview_submission_fps':self.rate(self.submit_times),
            'real_game_fps':None,'displayed_fps':None,'input_latency_ms':None,'gpu_utilization_percent':None,'vram_bytes':None,'power_watts':None,
            'generated_completed':self.generated,'blended_completed':self.blended,'repeated_submitted':self.repeated,'preview_submitted':self.submitted,'dropped':self.dropped,
            'capture_ms':self.capture_ms,'processing_ms':self.processing_ms,'cpu_core_percent':self.cpu,'process_tree_rss_sum_bytes':self.rss,
            'pipeline_ms':self.pipeline_ms,'lookahead_ms':self.lookahead_ms,'width':self.width,'height':self.height,
            'backend':self.backend,'device':self.device,'measurement_scope':'application_and_research_preview'}
    def sample_processes(self,engine=None):
        values=[];rss=0;sampled=0
        pids=[psutil.Process().pid]+([engine.process.pid] if engine and engine.process.poll() is None else [])
        for pid in pids:
            try:
                if pid not in self.processes:
                    self.processes[pid]=psutil.Process(pid);self.processes[pid].cpu_percent();rss+=self.processes[pid].memory_info().rss;sampled+=1;continue
                proc=self.processes[pid];values.append(proc.cpu_percent());rss+=proc.memory_info().rss;sampled+=1
            except psutil.Error:continue
        with self.lock:
            self.cpu=sum(values) if values else None;self.rss=rss if sampled else None

class Session:
    def __init__(self,binary,config,p,window):
        self.binary=binary;self.config=config;self.profile=dict(p);self.window=window
        self.output=queue.Queue(maxsize=1);self.errors=queue.Queue(maxsize=1);self.stop_event=threading.Event()
        self.engine=None;self.metrics=Metrics();self.thread=threading.Thread(target=self.run,name='EKA-capture',daemon=True)
    def start(self):self.thread.start()
    def run(self):
        capture=None
        try:
            capture=capture_class()(self.window)
            interval=1/effective_processing(self.profile)['fps'];deadline=time.perf_counter();last_capture=None;identity=0
            while not self.stop_event.is_set():
                if self.stop_event.wait(max(0,deadline-time.perf_counter())):break
                start=time.perf_counter();raw=capture.grab();image=prepare(raw,self.profile['aspect']);captured=time.perf_counter()
                if self.engine is None or self.engine.size!=image.size:
                    if self.engine:self.engine.close()
                    self.engine=EnginePipe(self.binary,self.config,self.profile,image.size)
                if self.stop_event.is_set():break
                data,images=self.engine.process_frame(image,identity,captured*1000)
                period=captured-last_capture if last_capture is not None else interval;last_capture=captured
                with self.metrics.lock:
                    self.metrics.capture_times.append(captured);self.metrics.capture_ms=(captured-start)*1000
                    self.metrics.processing_ms=data['processing_ms'];self.metrics.generated+=data['generated_count'];self.metrics.blended+=data.get('blended_count',0)
                    self.metrics.pipeline_ms=(time.perf_counter()-start)*1000;self.metrics.lookahead_ms=data['lookahead_ms']
                    self.metrics.width,self.metrics.height=image.size
                    self.metrics.backend=data['backend'];self.metrics.device=data['device']
                batch=(data,images,period)
                try:self.output.put_nowait(batch)
                except queue.Full:
                    try:self.output.get_nowait()
                    except queue.Empty:pass
                    with self.metrics.lock:self.metrics.dropped+=1
                    self.output.put_nowait(batch)
                identity+=1;deadline=max(deadline+interval,time.perf_counter())
        except Exception as error:
            if not self.stop_event.is_set():
                try:self.errors.put_nowait(str(error) if str(error).startswith('desktop.') else 'desktop.error.capture_failed')
                except queue.Full:pass
        finally:
            if capture:capture.close()
            if self.engine:self.engine.close()
    def stop(self):
        self.stop_event.set()
        if self.engine and self.engine.process.poll() is None:self.engine.process.terminate()
    def join(self,timeout=4):self.thread.join(timeout)
