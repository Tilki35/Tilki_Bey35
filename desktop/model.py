"""Persistent profiles; no rendering or processing in the settings layer."""
import copy
import json
import os
from pathlib import Path
import sys
import tempfile
import uuid

def config_directory():
    if sys.platform=='win32':return Path(os.environ.get('LOCALAPPDATA',os.environ.get('APPDATA',Path.home())))/'UniFrame'
    if sys.platform=='darwin':return Path.home()/'Library/Application Support/UniFrame'
    xdg=Path(os.environ.get('XDG_CONFIG_HOME',Path.home()/'.config'))
    if not xdg.is_absolute():xdg=Path.home()/'.config'
    return xdg/'uniframe'

def atomic_json(path,data):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    temporary=None
    try:
        with tempfile.NamedTemporaryFile('w',encoding='utf-8',dir=path.parent,delete=False) as file:
            temporary=Path(file.name);json.dump(data,file,ensure_ascii=False,indent=2);file.flush();os.fsync(file.fileno())
        os.replace(temporary,path)
    finally:
        if temporary and temporary.exists():temporary.unlink()

def profile(name='Default',identity=None):
    return {'id':identity or str(uuid.uuid4()),'name':name,'target_title':'','mode':'adaptive',
            'multiplier':2,'quality':'balanced','power':'balanced','backend':'auto','search':'auto',
            'fps_limit':10,'blend_weight':.5,'artifact_reduction':True,'scaling':'off','aspect':'fit'}

class Profiles:
    def __init__(self,path):
        self.path=Path(path)
        if self.path.exists():
            try:self.data=json.loads(self.path.read_text(encoding='utf-8'));self.validate()
            except (ValueError,TypeError,KeyError):raise ValueError('desktop.error.profile_file') from None
        else:self.data={'version':1,'profiles':[profile(identity='default')],'selected':'default',
                        'advanced':False,'theme':'dark','accent':'#39d8ac','flag_mode':'auto','flag_path':''}
    def validate(self):
        if not isinstance(self.data,dict) or self.data.get('version')!=1:raise ValueError()
        profiles=self.data['profiles']
        if not isinstance(profiles,list) or not profiles:raise ValueError()
        ids=set()
        for p in profiles:
            if not isinstance(p,dict) or not isinstance(p.get('name'),str) or not p['name'].strip():raise ValueError()
            if not isinstance(p.get('id'),str) or p['id'] in ids:raise ValueError()
            ids.add(p['id'])
            for key,values in {'mode':['adaptive','interpolate','repeat','blend','off'],
                 'quality':['eco','balanced','performance','quality'],'power':['auto','eco','balanced','performance','quality'],
                 'backend':['auto','cpu','vulkan'],'search':['auto','standard','high'],
                 'scaling':['off','spatial'],'aspect':['fit','fill']}.items():
                if p.get(key) not in values:raise ValueError()
            if not isinstance(p.get('target_title'),str):raise ValueError()
            if type(p.get('multiplier')) is not int or not 1<=p['multiplier']<=8:raise ValueError()
            if type(p.get('fps_limit')) not in (float,int) or not 1<=p['fps_limit']<=60:raise ValueError()
            if type(p.get('blend_weight')) not in (float,int) or not 0<=p['blend_weight']<=1:raise ValueError()
            if type(p.get('artifact_reduction')) is not bool or type(p.get('preview_pacing',True)) is not bool:raise ValueError()
        if 'default' not in ids or self.data.get('selected') not in ids:raise ValueError()
        if self.data.get('theme') not in ['dark','light']:raise ValueError()
        accent=self.data.get('accent','')
        if len(accent)!=7 or accent[0]!='#' or any(c not in '0123456789abcdefABCDEF' for c in accent[1:]):raise ValueError()
        if self.data.get('flag_mode') not in ['auto','on','off']:raise ValueError()
        if not isinstance(self.data.get('flag_path'),str) or type(self.data.get('advanced')) is not bool:raise ValueError()
    @property
    def current(self):return next(p for p in self.data['profiles'] if p['id']==self.data['selected'])
    def save(self):self.validate();atomic_json(self.path,self.data)
    def add(self,name,duplicate=False):
        if not name.strip():raise ValueError('desktop.error.profile_name')
        p=copy.deepcopy(self.current) if duplicate else profile()
        p.update(id=str(uuid.uuid4()),name=name.strip()[:128]);self.data['profiles'].append(p);self.data['selected']=p['id'];self.save();return p
    def remove(self):
        if self.current['id']=='default':raise ValueError('desktop.error.default_profile')
        self.data['profiles'].remove(self.current);self.data['selected']='default';self.save()

def effective_processing(p):
    """Presets change real computation/capture limits; they do not claim measured watts."""
    power=p['power'];quality=p['quality']
    fps=min(p['fps_limit'],5 if power=='eco' else 15 if power=='performance' else 10 if power in ['balanced','auto'] else 12)
    radius=8 if p['search']=='standard' else 16 if p['search']=='high' else {'eco':8,'performance':10,'balanced':12,'quality':16}[quality]
    if power=='eco':radius=min(radius,8)
    return {'mode':'repeat' if p['mode']=='off' else p['mode'],'multiplier':1 if p['mode']=='off' else p['multiplier'],
            'estimator':'optical-flow' if quality=='quality' and power!='eco' else 'block',
            'radius':radius,'fps':fps,'backend':p['backend'],'weight':p['blend_weight']}
