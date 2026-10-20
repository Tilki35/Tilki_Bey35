"""Integration tests for names, settings, Unicode resources and locale precedence."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
binary=root/'dist'/('tilki-fg.exe' if os.name=='nt' else 'tilki-fg')
with tempfile.TemporaryDirectory(prefix='uniframe-l10n-') as tmp:
    base=Path(tmp)
    env=dict(os.environ,LC_ALL='C',LC_MESSAGES='',LANG='C',LANGUAGE='')
    def command(*args,config=None,environment=None,expected=0,input=None):
        config=config or base/'settings.json'
        process=subprocess.run([str(binary),'--config',str(config),*args],env=environment or env,
                               capture_output=True,text=True,encoding='utf-8',input=input,timeout=20)
        assert process.returncode==expected,(args,process.stdout,process.stderr)
        return process
    def settings(*args,**kwargs):return json.loads(command(*args,'--settings',**kwargs).stdout)
    names_data=json.loads((root/'locales/names.json').read_text())
    for language in ('tr-TR','az-AZ','uz-UZ','tk-TM','ky-KG','ug-CN','tt-RU','ba-RU','kaa-UZ','cv-RU','tyv-RU','alt-RU'):
        data=settings('--locale',language)
        assert data['program_name']==names_data['languages'][language.split('-')[0]],data
        if language!='tr-TR':assert data['local_name_metadata']['name_verification']=='user_proposed_unverified'
    for language in ('kk-KZ','kaz_KZ.UTF-8'):
        assert settings('--locale',language)['program_name']=='ӘмКадр'
    for language in ('en-US','en-GB','de-DE','fr-FR','es-ES','ru-RU','ja-JP','ko-KR','zh-CN','zh-TW',
                     'ar-SA','hi-IN','eo','la','sah-RU','tlh','quc-GT','sw-KE','bn-BD','am-ET'):
        data=settings('--locale',language)
        assert data['program_name']==names_data['languages'].get(language.split('-')[0],'UniFrame') and data['language_well_formed'],data
    data=settings('--locale','tr-TR','--ui-language','en','--name','auto')
    assert data['program_name']=='EVRKare' and data['ui_language_effective']=='en'
    data=settings('--locale','en-US','--name','EVRKare','--ui-language','tr')
    assert data['program_name']=='EVRKare' and data['ui_language_effective']=='tr'
    data=settings('--locale','fr-FR')
    assert data['program_name']=='EVRKare' and data['ui_language_effective']=='tr','Preferences must persist independently'
    for name in ('UniFrame','XFrame','ӘмКадр'):
        assert settings('--name',name,'--locale','tr-TR')['program_name']==name
    data=settings('--name','auto','--ui-language','auto','--locale','C')
    assert data['program_name']=='XFrame' and data['detected_language'] is None
    data=settings('--locale','ar-SA')
    assert data['os_text_direction']=='rtl' and data['ui_language_effective']=='en' and data['ui_text_direction']=='ltr'
    assert [l['tag'] for l in data['available_translations']]==['en','tr']
    if os.name=='posix' and os.uname().sysname=='Linux':
        data=settings(environment=dict(env,LC_ALL='tr_TR.UTF-8',LANGUAGE='de_DE:fr_FR'))
        assert data['detected_language']=='de-DE' and data['locale_source']=='environment:LANGUAGE'
        data=settings(environment=dict(env,LC_ALL='C',LANGUAGE='tr:en'))
        assert data['detected_language'] is None and data['program_name']=='XFrame'
        data=settings(environment=dict(env,LC_ALL='',LC_MESSAGES='kk_KZ.UTF-8',LANG='en_US.UTF-8'))
        assert data['program_name']=='ӘмКадр'
    # External catalogs and name rules are picked up without compiling source code.
    resources=base/'resources';resources.mkdir()
    arabic={'_meta':{'language':'ar','native_name':'العربية','direction':'rtl'},
            'messages':{'app.description':'{name} — اختبار','settings.saved':'تم الحفظ'}}
    (resources/'ar.json').write_text(json.dumps(arabic,ensure_ascii=False),encoding='utf-8')
    data=settings('--resources',str(resources),'--locale','ar-SA')
    assert data['ui_language_effective']=='ar' and data['ui_text_direction']=='rtl'
    assert next(l for l in data['available_translations'] if l['tag']=='ar')['fallback_message_count']>0
    help=command('--resources',str(resources),'--locale','ar-SA','--help').stdout
    assert 'UniFrame' in help and 'offline frame generation' in help,'Missing Arabic help must fall back to English'
    (resources/'ar.json').write_text('{invalid',encoding='utf-8')
    data=settings('--resources',str(resources),'--locale','ar-SA')
    assert data['ui_language_effective']=='en' and 'ar.json' in data['invalid_resource_files']
    names=json.loads((root/'locales/names.json').read_text())
    names['languages']['kk']='ЖаңаКадр'
    (resources/'names.json').write_text(json.dumps(names,ensure_ascii=False),encoding='utf-8')
    assert settings('--resources',str(resources),'--locale','kk-KZ')['program_name']=='ЖаңаКадр'
    # Bad input must not overwrite saved preferences or silently discard corrupt files.
    original=(base/'settings.json').read_bytes()
    for args in [('--name','wrong'),('--ui-language','../tr'),('--locale','en--US'),('--name',)]:
        command(*args,expected=1)
        assert (base/'settings.json').read_bytes()==original
    corrupt=base/'corrupt.json';corrupt.write_text('{broken')
    command('--settings',config=corrupt,expected=1)
    assert corrupt.read_text()=='{broken'
    data=settings('--reset-settings','--locale','tr-TR',config=corrupt)
    assert data['program_name']=='EVRKare'
    menu=base/'menu.json'
    command('--settings-menu',config=menu,input='4\n3\n')
    data=settings('--locale','en-US',config=menu)
    assert data['program_name']=='XFrame' and data['ui_language_effective']=='tr'
    original=menu.read_bytes()
    command('--settings-menu',config=menu,input='2\n99\n',expected=1)
    assert menu.read_bytes()==original
    (base/'settings.json').write_text(json.dumps({'name':'auto','ui_language':'auto','future_key':'preserved'}))
    settings('--name','UniFrame')
    assert json.loads((base/'settings.json').read_text())['future_key']=='preserved'
print('PASS: worldwide names, independent persisted preferences, Unicode, external catalogs/rules, English fallback, RTL metadata, menu and corrupt-file preservation.')
