"""Original Tk desktop UI. Run: python -m desktop.app (requires Tk, Pillow, psutil)."""
from collections import deque
import copy
import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import time
import tkinter as tk
from tkinter import ttk,messagebox,simpledialog,filedialog,colorchooser
from tkinter import font as tkfont
from PIL import Image,ImageTk
from .model import Profiles,config_directory,atomic_json
from .capture import windows,CaptureError
from .runtime import Session,Metrics,creation_flags

ROOT=Path(__file__).resolve().parents[1]
def binary_path():
    filename='UniFrame.exe' if sys.platform=='win32' else 'uniframe'
    for candidate in [ROOT/filename,ROOT/'dist'/filename]:
        if candidate.is_file():return candidate
    raise RuntimeError('desktop.error.engine_missing')

class RoundedCard(tk.Canvas):
    def __init__(self,parent,app,title):
        super().__init__(parent,bg=app.colors['background'],highlightthickness=0,height=190)
        self.app=app;self.inner=tk.Frame(self,bg=app.colors['card']);self.window=self.create_window(14,12,window=self.inner,anchor='nw')
        ttk.Label(self.inner,text=title,style='CardTitle.TLabel').pack(anchor='w',pady=(0,12))
        self.bind('<Configure>',self.resize);self.inner.bind('<Configure>',lambda _:self.configure(height=self.inner.winfo_reqheight()+28))
    def resize(self,event):
        self.delete('shape');w=event.width;h=event.height;r=14
        self.create_polygon(r,0,w-r,0,w,r,w,h-r,w-r,h,r,h,0,h-r,0,r,smooth=True,fill=self.app.colors['card'],outline='',tags='shape')
        self.tag_lower('shape');self.itemconfigure(self.window,width=max(1,w-28))

class App:
    def __init__(self,root,config=None):
        self.root=root;self.directory=Path(config) if config else config_directory();self.directory.mkdir(parents=True,exist_ok=True)
        self.font_family=tkfont.nametofont('TkDefaultFont').actual('family')
        self.config=self.directory/'settings.json';self.profiles=Profiles(self.directory/'desktop.json');self.binary=binary_path()
        self.session=None;self.metrics=Metrics();self.pending=deque();self.preview=None;self.preview_image=None;self.page='home';self.window_items=[]
        self.catalogs={}
        for path in (ROOT/'locales').glob('*.json'):
            if path.name=='names.json':continue
            try:
                messages=json.loads(path.read_text(encoding='utf-8'))['messages']
                if isinstance(messages,dict):self.catalogs[path.stem.casefold()]=messages
            except (ValueError,KeyError,OSError):continue
        self.info=self.native_settings();self.language=self.info['ui_language_effective'];self.name=self.info['program_name'];self.fields=[]
        self.root.title(self.name);self.root.geometry('1200x800');self.root.minsize(900,640);self.root.protocol('WM_DELETE_WINDOW',self.close)
        self.root.bind('<Control-Alt-s>',lambda _:self.toggle());self.root.bind('<Escape>',lambda _:self.stop() if self.session else None)
        self.status=tk.StringVar(value=self.t('idle'));self.live_summary=tk.StringVar(value=self.t('quick_help'));self.measure_vars={};self.rebuild();self.root.after(20,self.poll);self.root.after(1000,self.sample)
    def t(self,key):
        if not key.startswith('desktop.'):key='desktop.'+key
        return self.catalogs.get(self.language.casefold(),{}).get(key,self.catalogs['en'].get(key,key))
    def native_settings(self,*args):
        command=[str(self.binary),'--config',str(self.config),'--resources',str(ROOT/'locales'),*args,'--settings']
        result=subprocess.run(command,capture_output=True,text=True,encoding='utf-8',timeout=15,**creation_flags())
        if result.returncode:raise RuntimeError(result.stderr.strip())
        return json.loads(result.stdout)
    def error(self,error):messagebox.showerror(self.name,self.t(str(error)),parent=self.root)
    def guard(self):
        if self.session:self.error('error.stop_first');return False
        return True
    def rebuild(self):
        for child in self.root.winfo_children():
            if not isinstance(child,tk.Toplevel):child.destroy()
        light=self.profiles.data['theme']=='light'
        self.colors={'background':'#edf2f6' if light else '#17191d','card':'#ffffff' if light else '#23262c',
                     'sidebar':'#dfe8ed' if light else '#111317','text':'#172b39' if light else '#e7f0f6',
                     'muted':'#526777' if light else '#96aebf','accent':self.profiles.data['accent']}
        c=self.colors;self.root.configure(bg=c['background']);style=ttk.Style(self.root);style.theme_use('clam')
        self.rtl=self.info.get('ui_text_direction')=='rtl'
        style.configure('.',font=(self.font_family,11),background=c['card'],foreground=c['text'])
        style.configure('TFrame',background=c['card']);style.configure('TLabel',background=c['card'],foreground=c['text'],anchor='e' if self.rtl else 'w')
        style.configure('TButton',padding=(12,8),background=c['card'],foreground=c['text'],borderwidth=0,relief='flat');style.map('TButton',background=[('active',c['accent'])],foreground=[('active','#071c18')])
        style.configure('Start.TButton',font=(self.font_family,13,'bold'),padding=(30,12),background=c['accent'],foreground='#08261d')
        style.configure('TCombobox',fieldbackground=c['card'],background=c['card'],foreground=c['text'],padding=5)
        style.map('TCombobox',fieldbackground=[('readonly',c['card'])],foreground=[('readonly',c['text'])])
        style.configure('CardTitle.TLabel',font=(self.font_family,12,'bold'));style.configure('Muted.TLabel',foreground=c['muted'])
        self.sidebar=tk.Frame(self.root,bg=c['sidebar'],width=230);self.sidebar.pack(side='right' if self.rtl else 'left',fill='y');self.sidebar.pack_propagate(False)
        tk.Label(self.sidebar,text=self.name,bg=c['sidebar'],fg=c['accent'],font=(self.font_family,20,'bold'),wraplength=210).pack(anchor='w',padx=18,pady=(24,5))
        tk.Label(self.sidebar,text=self.t('edition'),bg=c['sidebar'],fg=c['muted'],font=(self.font_family,8)).pack(anchor='w',padx=18)
        for page in ['home','performance','guide','world','settings']:
            ttk.Button(self.sidebar,text=self.t(page),command=lambda p=page:self.navigate(p)).pack(fill='x',padx=12,pady=3)
        tk.Label(self.sidebar,text=self.t('profiles'),bg=c['sidebar'],fg=c['muted']).pack(anchor='w',padx=16,pady=(22,8))
        self.profile_list=tk.Listbox(self.sidebar,bg=c['sidebar'],fg=c['text'],selectbackground=c['accent'],selectforeground='#08261d',borderwidth=0,highlightthickness=0,height=9,exportselection=False)
        self.profile_list.pack(fill='both',expand=True,padx=12)
        for i,p in enumerate(self.profiles.data['profiles']):
            self.profile_list.insert('end',self.t('default') if p['id']=='default' and p['name']=='Default' else p['name'])
            if p['id']==self.profiles.data['selected']:self.profile_list.selection_set(i)
        self.profile_list.bind('<<ListboxSelect>>',self.select_profile)
        if self.session:self.profile_list.configure(state='disabled')
        for text,method in [('new',self.new_profile),('edit',self.edit_profile),('copy',lambda:self.new_profile(True)),('delete',self.delete_profile)]:
            ttk.Button(self.sidebar,text=self.t(text),command=method,state='disabled' if self.session or (text=='delete' and self.profiles.current['id']=='default') else 'normal').pack(fill='x',padx=12,pady=2)
        self.main=tk.Frame(self.root,bg=c['background']);self.main.pack(side='left',fill='both',expand=True,padx=22,pady=20)
        self.toolbar=tk.Frame(self.main,bg=c['background']);self.toolbar.pack(fill='x')
        tk.Label(self.toolbar,text=self.profiles.current['name'] if self.profiles.current['name']!='Default' else self.t('default'),bg=c['background'],fg=c['text'],font=(self.font_family,20,'bold')).pack(side='left')
        self.start_button=ttk.Button(self.toolbar,text=self.t('stop' if self.session else 'start'),style='Start.TButton',command=self.toggle);self.start_button.pack(side='left' if self.rtl else 'right')
        tk.Label(self.main,textvariable=self.live_summary,bg=c['background'],fg=c['accent'],anchor='w',wraplength=850).pack(fill='x',pady=(10,0))
        self.advanced=tk.BooleanVar(value=self.profiles.data['advanced'])
        ttk.Checkbutton(self.main,text=self.t('advanced'),variable=self.advanced,command=self.change_view).pack(anchor='w',pady=12)
        tk.Label(self.main,text=self.t('preview_notice'),bg=c['background'],fg=c['muted'],anchor='w',wraplength=850).pack(fill='x',pady=(0,12))
        self.body_canvas=tk.Canvas(self.main,bg=c['background'],highlightthickness=0);scroll=ttk.Scrollbar(self.main,orient='vertical',command=self.body_canvas.yview)
        scroll.pack(side='right',fill='y');self.body_canvas.pack(fill='both',expand=True);self.body_canvas.configure(yscrollcommand=scroll.set)
        self.body=tk.Frame(self.body_canvas,bg=c['background']);self.body_window=self.body_canvas.create_window(0,0,window=self.body,anchor='nw')
        self.body.bind('<Configure>',lambda _:self.body_canvas.configure(scrollregion=self.body_canvas.bbox('all')))
        self.body_canvas.bind('<Configure>',self.layout)
        self.root.bind('<MouseWheel>',lambda e:self.scroll_body(-1 if e.delta>0 else 1))
        self.root.bind('<Button-4>',lambda e:self.scroll_body(-1))
        self.root.bind('<Button-5>',lambda e:self.scroll_body(1))
        tk.Label(self.main,textvariable=self.status,bg=c['background'],fg=c['muted'],anchor='w',wraplength=850).pack(fill='x',pady=(12,0))
        self.cards=[];self.fields=[];self.render_page()
        self.show_flag()
    def scroll_body(self,direction):
        if self.body_canvas.winfo_exists():self.body_canvas.yview_scroll(direction*3,'units')
    def show_flag(self):
        path=self.profiles.data.get('flag_path','');mode=self.profiles.data['flag_mode']
        naming=json.loads((ROOT/'locales/names.json').read_text(encoding='utf-8'));language=self.info.get('detected_language') or ''
        if self.info.get('ui_language_requested')!='auto':language=self.info['ui_language_requested']
        family=naming.get('metadata',{}).get(language.split('-')[0],{}).get('family')
        if path and mode!='off' and (mode=='on' or family=='turkic'):
            try:
                with Image.open(path) as image:self.flag_image=ImageTk.PhotoImage(image.convert('RGB').resize((80,48)))
                tk.Label(self.sidebar,image=self.flag_image,bg=self.colors['sidebar']).pack(pady=10)
            except (OSError,ValueError):pass
    def layout(self,event):
        self.body_canvas.itemconfigure(self.body_window,width=event.width)
        columns=2 if event.width>=760 else 1
        for i,card in enumerate(self.cards):card.grid(row=i//columns,column=i%columns,sticky='new',padx=(0,12),pady=(0,12))
        for col in range(2):self.body.columnconfigure(col,weight=1 if col<columns else 0,minsize=0)
    def card(self,key):
        card=RoundedCard(self.body,self,self.t(key));self.cards.append(card);return card.inner
    def combo(self,parent,key,values,labels=None,profile=True):
        ttk.Label(parent,text=self.t(key),style='Muted.TLabel').pack(anchor='w')
        labels=labels or [self.t(key+'.'+str(v)) for v in values];current=self.profiles.current.get(key)
        variable=tk.StringVar(value=labels[values.index(current)] if current in values else labels[0])
        widget=ttk.Combobox(parent,textvariable=variable,values=labels,state='disabled' if self.session else 'readonly',width=24,justify='right' if self.rtl else 'left');widget.pack(fill='x',pady=(4,10))
        def update(_):
            if not self.guard():return
            self.profiles.current[key]=values[labels.index(variable.get())];self.profiles.save()
            if key=='mode':self.navigate('home')
        widget.bind('<<ComboboxSelected>>',update);self.fields.append(widget);return widget
    def render_page(self):
        if self.page=='home':self.home()
        elif self.page=='performance':self.performance()
        elif self.page=='settings':self.settings()
        elif self.page=='guide':ttk.Label(self.card('guide'),text=self.t('guide_text'),wraplength=650,justify='left').pack(anchor='w')
        else:self.world()
        self.root.after_idle(lambda:self.layout(type('Size',(),{'width':max(self.body_canvas.winfo_width(),500)})()))
    def home(self):
        game=self.card('game');self.target=ttk.Combobox(game,state='readonly',width=26);self.target.pack(fill='x',pady=(0,8));self.target.bind('<<ComboboxSelected>>',self.target_changed)
        ttk.Button(game,text=self.t('refresh'),command=self.refresh_windows,state='disabled' if self.session else 'normal').pack(anchor='w')
        if self.session:self.target.configure(values=[self.profiles.current['target_title']],state='disabled');self.target.set(self.profiles.current['target_title'])
        else:self.refresh_windows(quiet=True)
        frame=self.card('generator');self.combo(frame,'mode',['adaptive','interpolate','repeat','blend','off']);multiplier=self.combo(frame,'multiplier',[1,2,3,4,6,8],[str(i)+'×' for i in [1,2,3,4,6,8]])
        quality=self.combo(frame,'quality',['eco','balanced','performance','quality'])
        ttk.Label(frame,text=self.t('mode_help.'+self.profiles.current['mode']),wraplength=320,style='Muted.TLabel').pack(anchor='w')
        if self.profiles.current['mode']=='blend':
            ttk.Label(frame,text=self.t('weight')).pack(anchor='w',pady=(8,0))
            weight=tk.DoubleVar(value=self.profiles.current['blend_weight'])
            slider=ttk.Scale(frame,from_=0,to=1,variable=weight,command=lambda v:self.slider('blend_weight',float(v)));slider.pack(fill='x',pady=8)
            if self.session:slider.configure(state='disabled')
        if self.profiles.current['mode']=='off':multiplier.configure(state='disabled')
        if self.profiles.current['mode'] in ['off','repeat','blend']:quality.configure(state='disabled')
        power=self.card('power');self.combo(power,'power',['auto','eco','balanced','performance','quality']);ttk.Label(power,text=self.t('power_note'),wraplength=300,style='Muted.TLabel').pack(anchor='w')
        scaling=self.card('scaling');self.combo(scaling,'scaling',['off','spatial']);self.combo(scaling,'aspect',['fit','fill'])
        ttk.Label(scaling,text=self.t('scaling_hint'),wraplength=300,style='Muted.TLabel').pack(anchor='w')
        if not self.advanced.get():return
        blend=self.card('blend');ttk.Label(blend,text=self.t('weight')).pack(anchor='w');var=tk.DoubleVar(value=self.profiles.current['blend_weight'])
        scale=ttk.Scale(blend,from_=0,to=1,variable=var,command=lambda value:self.slider('blend_weight',float(value)));scale.pack(fill='x',pady=12)
        if self.profiles.current['mode']!='blend' or self.session:scale.configure(state='disabled')
        capture=self.card('capture');api='Windows GDI BitBlt' if sys.platform=='win32' else 'X11 XGetImage' if sys.platform.startswith('linux') else '—'
        ttk.Label(capture,text=self.t('capture_api')+': '+api).pack(anchor='w');ttk.Label(capture,text=self.t('queue'),wraplength=300,style='Muted.TLabel').pack(anchor='w',pady=8)
        ttk.Label(capture,text=self.t('fps_limit')).pack(anchor='w');fps=ttk.Scale(capture,from_=1,to=60,command=lambda value:self.slider('fps_limit',int(float(value))));fps.set(self.profiles.current['fps_limit']);fps.pack(fill='x',pady=8)
        if self.session:fps.configure(state='disabled')
        renderer=self.card('renderer');self.combo(renderer,'backend',['auto','cpu','vulkan']);search=self.combo(renderer,'search',['auto','standard','high'])
        if self.profiles.current['mode'] in ['off','repeat','blend']:search.configure(state='disabled')
        self.protect=tk.BooleanVar(value=self.profiles.current['artifact_reduction'])
        protect=ttk.Checkbutton(renderer,text=self.t('artifact_protection'),variable=self.protect,command=lambda:self.slider('artifact_reduction',self.protect.get()));protect.pack(anchor='w')
        if self.profiles.current['mode'] in ['off','repeat','blend'] or self.session:protect.configure(state='disabled')
        self.pacing=tk.BooleanVar(value=self.profiles.current.get('preview_pacing',True));ttk.Checkbutton(renderer,text=self.t('pacing'),variable=self.pacing,command=lambda:self.slider('preview_pacing',self.pacing.get())).pack(anchor='w')
        ttk.Label(renderer,text=self.t('unsupported'),wraplength=300,style='Muted.TLabel').pack(anchor='w',pady=8)
    def slider(self,key,value):
        if not self.session:self.profiles.current[key]=value
        if hasattr(self,'save_after'):
            try:self.root.after_cancel(self.save_after)
            except tk.TclError:pass
        self.save_after=self.root.after(250,self.profiles.save)
    def performance(self):
        card=self.card('performance')
        for key in ['capture_fps','preview_fps','game_fps','displayed_fps','latency','generated','blended','repeated','dropped','processing_ms','pipeline_ms','lookahead_ms','resolution','cpu','ram','gpu','device']:
            row=ttk.Frame(card);row.pack(fill='x',pady=5);ttk.Label(row,text=self.t(key),wraplength=260).pack(side='left')
            variable=self.measure_vars.setdefault(key,tk.StringVar(value=self.t('unknown')));ttk.Label(row,textvariable=variable).pack(side='right')
        ttk.Button(card,text=self.t('export'),command=self.export).pack(anchor='w',pady=12)
    def settings(self):
        card=self.card('settings');names=[self.t('auto'),'EVRKare','UniFrame','XFrame','ӘмКадр'];self.brand=tk.StringVar(value=self.t('auto') if self.info['name_preference']=='auto' else self.info['name_preference'])
        ttk.Label(card,text=self.t('program_name')).pack(anchor='w');ttk.Combobox(card,textvariable=self.brand,values=names,state='readonly').pack(fill='x',pady=8)
        self.language_choices={self.t('auto'):'auto',**{x['native_name']:x['tag'] for x in self.info['available_translations']}}
        current=next((key for key,value in self.language_choices.items() if value==self.info['ui_language_requested']),self.t('auto'))
        self.ui_language=tk.StringVar(value=current);languages=list(self.language_choices)
        ttk.Label(card,text=self.t('ui_language')).pack(anchor='w');ttk.Combobox(card,textvariable=self.ui_language,values=languages,state='readonly').pack(fill='x',pady=8)
        ttk.Button(card,text=self.t('apply'),command=self.apply_language).pack(anchor='w')
        theme=self.card('theme');value=tk.StringVar(value=self.profiles.data['theme']);widget=ttk.Combobox(theme,textvariable=value,values=['dark','light'],state='readonly');widget.pack(fill='x',pady=8)
        widget.bind('<<ComboboxSelected>>',lambda _:self.appearance('theme',value.get()));ttk.Button(theme,text=self.t('accent'),command=self.change_accent).pack(anchor='w')
        flag=self.card('flag');value=tk.StringVar(value=self.profiles.data['flag_mode']);widget=ttk.Combobox(flag,textvariable=value,values=['auto','on','off'],state='readonly');widget.pack(fill='x',pady=8)
        widget.bind('<<ComboboxSelected>>',lambda _:self.appearance('flag_mode',value.get()));ttk.Button(flag,text=self.t('choose'),command=self.choose_flag).pack(anchor='w')
        ttk.Label(flag,text=self.t('flag_note'),wraplength=300).pack(anchor='w',pady=10)
        ttk.Label(self.card('about'),text=self.t('about'),wraplength=330,justify='left').pack(anchor='w')
    def world(self):
        card=self.card('world');ttk.Label(card,text=self.t('world_note'),wraplength=650).pack(anchor='w',pady=8)
        data=json.loads((ROOT/'locales/names.json').read_text(encoding='utf-8'));likely=json.loads((ROOT/'locales/cldr/likelySubtags.json').read_text())['supplemental']['likelySubtags']
        ttk.Label(card,text=self.t('world_columns'),wraplength=650,style='Muted.TLabel').pack(anchor='w',pady=8)
        for tag,name in data['languages'].items():
            if data.get('metadata',{}).get(tag,{}).get('family')!='turkic':continue
            script=likely.get(tag,'und-Unknown-001').split('-')[1]
            ttk.Label(card,text=f"{tag}  ·  {name}  ·  {script}  ·  "+self.t('translated' if tag=='tr' else 'not_translated')+'  ·  '+self.t('official' if tag=='tr' else 'proposed'),wraplength=650).pack(anchor='w',pady=5)
    def navigate(self,page):
        self.page=page
        for child in self.body.winfo_children():child.destroy()
        self.cards=[];self.fields=[];self.render_page()
    def refresh_windows(self,quiet=False):
        if self.session:return
        try:self.window_items=[item for item in windows() if not item.title.startswith(self.name)]
        except CaptureError as e:
            self.window_items=[]
            if not quiet:self.error(e)
        self.target['values']=[item.title for item in self.window_items]
        self.target.set(self.profiles.current.get('target_title',''))
    def target_changed(self,_=None):
        if not self.guard():return
        self.profiles.current['target_title']=self.target.get();self.profiles.save()
    def select_profile(self,_=None):
        if not self.guard():return
        selected=self.profile_list.curselection()
        if selected:self.profiles.data['selected']=self.profiles.data['profiles'][selected[0]]['id'];self.profiles.save();self.rebuild()
    def new_profile(self,duplicate=False):
        if not self.guard():return
        name=simpledialog.askstring(self.name,self.t('profile_name'),parent=self.root)
        if name:
            try:self.profiles.add(name,duplicate);self.rebuild()
            except ValueError as e:self.error(e)
    def edit_profile(self):
        if not self.guard():return
        name=simpledialog.askstring(self.name,self.t('profile_name'),initialvalue=self.profiles.current['name'],parent=self.root)
        if name:self.profiles.current['name']=name.strip()[:128];self.profiles.save();self.rebuild()
    def delete_profile(self):
        if not self.guard():return
        if messagebox.askyesno(self.name,self.t('delete_confirm'),parent=self.root):
            try:self.profiles.remove();self.rebuild()
            except ValueError as e:self.error(e)
    def change_view(self):self.profiles.data['advanced']=self.advanced.get();self.profiles.save();self.navigate(self.page)
    def appearance(self,key,value):self.profiles.data[key]=value;self.profiles.save();self.rebuild()
    def change_accent(self):
        color=colorchooser.askcolor(self.profiles.data['accent'],parent=self.root)[1]
        if color:self.appearance('accent',color)
    def choose_flag(self):
        path=filedialog.askopenfilename(parent=self.root,filetypes=[('PNG / JPEG','*.png *.jpg *.jpeg')])
        if path:
            try:
                with Image.open(path) as image:
                    if image.width*image.height>16000000:raise ValueError()
                    target=self.directory/'user-flag.png';image=image.convert('RGB');image.thumbnail((640,400));image.save(target)
                self.appearance('flag_path',str(target))
            except (ValueError,OSError):self.error('error.capture_failed')
    def apply_language(self):
        if not self.guard():return
        try:self.info=self.native_settings('--name','auto' if self.brand.get()==self.t('auto') else self.brand.get(),'--ui-language',self.language_choices[self.ui_language.get()]);self.language=self.info['ui_language_effective'];self.name=self.info['program_name'];self.root.title(self.name);self.rebuild()
        except RuntimeError as e:self.error(e)
    def toggle(self):
        if self.session:self.stop();return
        try:
            candidates=[window for window in windows() if window.title==self.profiles.current['target_title'] and not window.title.startswith(self.name)]
            if not candidates:raise CaptureError('desktop.error.select_window')
            self.session=Session(self.binary,self.config,self.profiles.current,candidates[0].id);self.metrics=self.session.metrics
            self.preview=tk.Toplevel(self.root);self.preview.title(self.name+' · '+self.t('preview'));self.preview.protocol('WM_DELETE_WINDOW',self.stop)
            ttk.Label(self.preview,text=self.t('preview_notice'),wraplength=550).pack(padx=12,pady=10)
            self.preview.geometry('800x500');self.preview.minsize(320,240);self.preview.configure(bg='#000000')
            self.preview.bind('<F11>',self.toggle_fullscreen);self.preview.bind('<Escape>',self.leave_fullscreen)
            self.preview_label=tk.Label(self.preview,bg='#000000',borderwidth=0);self.preview_label.pack(fill='both',expand=True,padx=0,pady=0)
            self.fullscreen=False
            self.session.start();self.status.set(self.t('running'));self.rebuild()
        except (CaptureError,RuntimeError) as e:
            self.stop();self.error(e)
    def toggle_fullscreen(self,_=None):
        if self.preview:
            self.fullscreen=not self.fullscreen;self.preview.attributes('-fullscreen',self.fullscreen)
    def leave_fullscreen(self,_=None):
        if self.preview:self.fullscreen=False;self.preview.attributes('-fullscreen',False)
    def preview_frame(self,image):
        width=max(1,self.preview_label.winfo_width());height=max(1,self.preview_label.winfo_height())
        resample=Image.Resampling.BILINEAR if self.profiles.current['scaling']=='spatial' else Image.Resampling.NEAREST
        ratio=min(width/image.width,height/image.height)
        image=image.resize((max(1,int(image.width*ratio)),max(1,int(image.height*ratio))),resample)
        self.preview_image=ImageTk.PhotoImage(image);self.preview_label.configure(image=self.preview_image)
    def stop(self):
        if self.session:self.session.stop();self.stopping=self.session;self.session=None;self.stopping.join(timeout=3)
        self.pending.clear()
        if self.preview:
            try:self.preview.destroy()
            except tk.TclError:pass
            self.preview=None
        self.live_summary.set(self.t('quick_help'));self.status.set(self.t('idle'));self.start_button.configure(text=self.t('start'))
        self.rebuild()
    def poll(self):
        if self.session:
            try:error=self.session.errors.get_nowait();self.stop();self.error(error)
            except queue.Empty:pass
        if self.session:
            try:
                data,images,period=self.session.output.get_nowait();now=time.perf_counter()
                with self.metrics.lock:self.metrics.dropped+=len(self.pending)
                self.pending.clear()
                for index,kind in enumerate(data['kinds']):
                    image=images[0] if len(images)==1 else images[index]
                    due=now+index*period/max(1,data['slot_count']) if self.profiles.current.get('preview_pacing',True) else now
                    self.pending.append((due,kind,image))
            except queue.Empty:pass
            now=time.perf_counter()
            if self.pending and self.pending[0][0]<=now:
                due,kind,image=self.pending.popleft()
                if kind!='real' and now-due>.04:
                    with self.metrics.lock:self.metrics.dropped+=1
                elif self.preview:
                    self.preview_frame(image)
                    with self.metrics.lock:self.metrics.submit_times.append(now);self.metrics.submitted+=1;self.metrics.repeated+=kind=='repeat'
        self.root.after(8 if self.session else 250,self.poll)
    def sample(self):
        self.metrics.sample_processes(self.session.engine if self.session else None);data=self.metrics.snapshot()
        if self.session:
            capture=data.get('capture_fps') or 0;preview=data.get('preview_submission_fps') or 0
            self.live_summary.set(self.t('live_rates').format(capture=capture,preview=preview))
        else:self.live_summary.set(self.t('quick_help'))
        mapping={'capture_fps':'capture_fps','preview_fps':'preview_submission_fps','generated':'generated_completed','blended':'blended_completed','repeated':'repeated_submitted',
            'dropped':'dropped','processing_ms':'processing_ms','pipeline_ms':'pipeline_ms','lookahead_ms':'lookahead_ms','cpu':'cpu_core_percent','ram':'process_tree_rss_sum_bytes','device':'device'}
        for key,var in self.measure_vars.items():
            value=data.get(mapping.get(key,''))
            if key=='resolution' and data.get('width'):value=f"{data['width']} × {data['height']}"
            if key=='ram' and value is not None:value/=1024**2
            var.set(f'{value:.1f}' if isinstance(value,(int,float)) else value or self.t('unknown'))
        self.root.after(1000,self.sample)
    def export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.json',filetypes=[('JSON','*.json')])
        if path:atomic_json(path,self.metrics.snapshot())
    def close(self):
        self.stop()
        if hasattr(self,'stopping'):self.stopping.join()
        self.profiles.save();self.root.destroy()

def main():
    root=tk.Tk()
    try:App(root);root.mainloop()
    except Exception as error:
        messagebox.showerror('XFrame',str(error),parent=root);root.destroy();raise
if __name__=='__main__':main()
