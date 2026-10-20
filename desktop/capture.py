"""Explicit window capture: X11 TrueColor or Windows GDI. No process injection."""
import ctypes as C
from ctypes import wintypes as W
from dataclasses import dataclass
import os
import sys
from PIL import Image,ImageOps

class CaptureError(RuntimeError):pass
@dataclass
class Window:
    id:int
    title:str

def prepare(image,aspect='fit'):
    if aspect=='fill':return ImageOps.fit(image,(256,144),Image.Resampling.BILINEAR)
    return ImageOps.pad(image,(256,144),Image.Resampling.BILINEAR,color='black')

class XImage(C.Structure):
    _fields_=[('width',C.c_int),('height',C.c_int),('xoffset',C.c_int),('format',C.c_int),('data',C.c_void_p),
        ('byte_order',C.c_int),('bitmap_unit',C.c_int),('bitmap_bit_order',C.c_int),('bitmap_pad',C.c_int),
        ('depth',C.c_int),('bytes_per_line',C.c_int),('bits_per_pixel',C.c_int),
        ('red_mask',C.c_ulong),('green_mask',C.c_ulong),('blue_mask',C.c_ulong)]
class XAttributes(C.Structure):
    _fields_=[('x',C.c_int),('y',C.c_int),('width',C.c_int),('height',C.c_int),('border_width',C.c_int),('depth',C.c_int),
        ('visual',C.c_void_p),('root',C.c_ulong),('window_class',C.c_int),('bit_gravity',C.c_int),('win_gravity',C.c_int),
        ('backing_store',C.c_int),('backing_planes',C.c_ulong),('backing_pixel',C.c_ulong),('save_under',C.c_int),
        ('colormap',C.c_ulong),('map_installed',C.c_int),('map_state',C.c_int),('all_event_masks',C.c_long),
        ('your_event_mask',C.c_long),('do_not_propagate_mask',C.c_long),('override_redirect',C.c_int),('screen',C.c_void_p)]

class X11Capture:
    api='X11 XGetImage'
    def __init__(self,window=None):
        if not os.environ.get('DISPLAY'):raise CaptureError('desktop.error.x11_required')
        self.x=C.CDLL('libX11.so.6');self.window=window;self.error=False
        signatures={'XOpenDisplay':(C.c_void_p,[C.c_char_p]),'XDefaultRootWindow':(C.c_ulong,[C.c_void_p]),
            'XCloseDisplay':(C.c_int,[C.c_void_p]),'XFree':(C.c_int,[C.c_void_p]),
            'XGetWindowAttributes':(C.c_int,[C.c_void_p,C.c_ulong,C.POINTER(XAttributes)]),
            'XFetchName':(C.c_int,[C.c_void_p,C.c_ulong,C.POINTER(C.c_void_p)]),
            'XQueryTree':(C.c_int,[C.c_void_p,C.c_ulong,C.POINTER(C.c_ulong),C.POINTER(C.c_ulong),C.POINTER(C.POINTER(C.c_ulong)),C.POINTER(C.c_uint)]),
            'XGetImage':(C.POINTER(XImage),[C.c_void_p,C.c_ulong,C.c_int,C.c_int,C.c_uint,C.c_uint,C.c_ulong,C.c_int]),
            'XDestroyImage':(C.c_int,[C.POINTER(XImage)]),'XSync':(C.c_int,[C.c_void_p,C.c_int]),
            'XInternAtom':(C.c_ulong,[C.c_void_p,C.c_char_p,C.c_int]),
            'XGetWindowProperty':(C.c_int,[C.c_void_p,C.c_ulong,C.c_ulong,C.c_long,C.c_long,C.c_int,C.c_ulong,
                    C.POINTER(C.c_ulong),C.POINTER(C.c_int),C.POINTER(C.c_ulong),C.POINTER(C.c_ulong),C.POINTER(C.c_void_p)])}
        for name,(restype,argtypes) in signatures.items():fn=getattr(self.x,name);fn.restype=restype;fn.argtypes=argtypes
        self.display=self.x.XOpenDisplay(None)
        if not self.display:raise CaptureError('desktop.error.x11_required')
        self.handler_type=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_void_p)
        self.x.XSetErrorHandler.argtypes=[C.c_void_p];self.x.XSetErrorHandler.restype=C.c_void_p
        def handler(display,event):
            if display==self.display:self.error=True;return 0
            return self.handler_type(self.previous_handler)(display,event) if self.previous_handler else 0
        self.handler=self.handler_type(handler);self.previous_handler=self.x.XSetErrorHandler(C.cast(self.handler,C.c_void_p))
    def property(self,window,name):
        atom=self.x.XInternAtom(self.display,name.encode(),True)
        if not atom:return None
        actual=C.c_ulong();fmt=C.c_int();count=C.c_ulong();remaining=C.c_ulong();data=C.c_void_p()
        self.x.XGetWindowProperty(self.display,window,atom,0,4096,False,0,C.byref(actual),C.byref(fmt),C.byref(count),C.byref(remaining),C.byref(data))
        if not data.value:return None
        try:
            if fmt.value==32:return list(C.cast(data,C.POINTER(C.c_ulong))[:count.value])
            if fmt.value==8:return C.string_at(data,count.value)
        finally:self.x.XFree(data)
    def windows(self):
        root=self.x.XDefaultRootWindow(self.display);ids=self.property(root,'_NET_CLIENT_LIST')
        if ids is None:
            owner=C.c_ulong();parent=C.c_ulong();children=C.POINTER(C.c_ulong)();count=C.c_uint()
            if not self.x.XQueryTree(self.display,root,C.byref(owner),C.byref(parent),C.byref(children),C.byref(count)):return []
            try:ids=list(children[:count.value])
            finally:
                if children:self.x.XFree(children)
        found=[]
        for identity in ids:
            attrs=XAttributes()
            if not self.x.XGetWindowAttributes(self.display,identity,C.byref(attrs)) or attrs.map_state!=2:continue
            name=self.property(identity,'_NET_WM_NAME')
            if not isinstance(name,bytes):
                ptr=C.c_void_p()
                if not self.x.XFetchName(self.display,identity,C.byref(ptr)) or not ptr.value:continue
                try:name=C.string_at(ptr)
                finally:self.x.XFree(ptr)
            title=name.decode('utf-8','replace').strip()
            if title:found.append(Window(identity,title))
        return found
    def grab(self):
        self.error=False;attrs=XAttributes()
        if not self.x.XGetWindowAttributes(self.display,self.window,C.byref(attrs)) or attrs.map_state!=2:raise CaptureError('desktop.error.window_closed')
        if attrs.width<=0 or attrs.height<=0 or attrs.width*attrs.height>16777216:raise CaptureError('desktop.error.capture_size')
        image=self.x.XGetImage(self.display,self.window,0,0,attrs.width,attrs.height,C.c_ulong(-1).value,2)
        self.x.XSync(self.display,False)
        if not image or self.error:
            if image:self.x.XDestroyImage(image)
            raise CaptureError('desktop.error.window_closed')
        try:
            info=image.contents
            if info.bits_per_pixel!=32 or info.byte_order!=0 or info.red_mask!=0xff0000 or info.green_mask!=0xff00 or info.blue_mask!=255:raise CaptureError('desktop.error.pixel_format')
            data=C.string_at(info.data,info.bytes_per_line*info.height)
            return Image.frombytes('RGB',(info.width,info.height),data,'raw','BGRX',info.bytes_per_line,1)
        finally:self.x.XDestroyImage(image)
    def close(self):
        if self.display:
            self.x.XSync(self.display,False);self.x.XSetErrorHandler(self.previous_handler);self.x.XCloseDisplay(self.display);self.display=None

class WindowsCapture:
    api='Windows GDI BitBlt (not WGC)'
    def __init__(self,window=None):
        self.window=window;self.u=C.WinDLL('user32',use_last_error=True);self.g=C.WinDLL('gdi32',use_last_error=True)
        for library,name,result,args in [(self.u,'GetDC',C.c_void_p,[C.c_void_p]),(self.u,'ReleaseDC',C.c_int,[C.c_void_p,C.c_void_p]),
             (self.g,'CreateCompatibleDC',C.c_void_p,[C.c_void_p]),(self.g,'CreateCompatibleBitmap',C.c_void_p,[C.c_void_p,C.c_int,C.c_int]),
             (self.g,'SelectObject',C.c_void_p,[C.c_void_p,C.c_void_p]),(self.g,'DeleteDC',C.c_int,[C.c_void_p]),(self.g,'DeleteObject',C.c_int,[C.c_void_p]),
             (self.g,'BitBlt',C.c_int,[C.c_void_p,C.c_int,C.c_int,C.c_int,C.c_int,C.c_void_p,C.c_int,C.c_int,C.c_uint]),
             (self.g,'GetDIBits',C.c_int,[C.c_void_p,C.c_void_p,C.c_uint,C.c_uint,C.c_void_p,C.c_void_p,C.c_uint])]:
            fn=getattr(library,name);fn.restype=result;fn.argtypes=args
        self.u.GetClientRect.argtypes=[C.c_void_p,C.POINTER(W.RECT)];self.u.IsIconic.argtypes=[C.c_void_p]
        self.u.IsWindowVisible.argtypes=[C.c_void_p];self.u.GetWindowTextLengthW.argtypes=[C.c_void_p]
        self.u.GetWindowTextW.argtypes=[C.c_void_p,W.LPWSTR,C.c_int]
    def windows(self):
        found=[];callback_type=C.WINFUNCTYPE(C.c_int,C.c_void_p,C.c_void_p)
        def visit(hwnd,_):
            length=self.u.GetWindowTextLengthW(hwnd)
            if self.u.IsWindowVisible(hwnd) and length:
                title=C.create_unicode_buffer(length+1);self.u.GetWindowTextW(hwnd,title,length+1);found.append(Window(hwnd,title.value))
            return 1
        callback=callback_type(visit);self.u.EnumWindows.argtypes=[callback_type,C.c_void_p];self.u.EnumWindows(callback,None);return found
    def grab(self):
        rect=W.RECT()
        if self.u.IsIconic(self.window) or not self.u.GetClientRect(self.window,C.byref(rect)):raise CaptureError('desktop.error.window_closed')
        width,height=rect.right,rect.bottom
        if width<=0 or height<=0 or width*height>16777216:raise CaptureError('desktop.error.capture_size')
        dc=self.u.GetDC(self.window);mem=None;bitmap=None;old=None
        if not dc:raise CaptureError('desktop.error.capture_failed')
        try:
            mem=self.g.CreateCompatibleDC(dc);bitmap=self.g.CreateCompatibleBitmap(dc,width,height)
            if not mem or not bitmap:raise CaptureError('desktop.error.capture_failed')
            old=self.g.SelectObject(mem,bitmap)
            if not self.g.BitBlt(mem,0,0,width,height,dc,0,0,0x00CC0020):raise CaptureError('desktop.error.capture_failed')
            self.g.SelectObject(mem,old);old=None
            class Header(C.Structure):
                _fields_=[('size',W.DWORD),('width',W.LONG),('height',W.LONG),('planes',W.WORD),('bits',W.WORD),
                    ('compression',W.DWORD),('sizeImage',W.DWORD),('x',W.LONG),('y',W.LONG),('used',W.DWORD),('important',W.DWORD)]
            header=Header(C.sizeof(Header),width,-height,1,32,0,0,0,0,0,0);data=C.create_string_buffer(width*height*4)
            if self.g.GetDIBits(dc,bitmap,0,height,data,C.byref(header),0)!=height:raise CaptureError('desktop.error.capture_failed')
            return Image.frombytes('RGB',(width,height),data.raw,'raw','BGRX')
        finally:
            if old:self.g.SelectObject(mem,old)
            if bitmap:self.g.DeleteObject(bitmap)
            if mem:self.g.DeleteDC(mem)
            self.u.ReleaseDC(self.window,dc)
    def close(self):pass

def capture_class():
    if sys.platform=='win32':return WindowsCapture
    if sys.platform.startswith('linux'):return X11Capture
    raise CaptureError('desktop.error.platform_capture')

def windows():
    capture=capture_class()()
    try:return capture.windows()
    finally:capture.close()
