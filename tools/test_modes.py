"""Exercise selectable modes, optical flow improvement, budget decisions and timing provenance."""
import json
import os
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[1]
binary=root/'dist'/('tilki-fg.exe' if os.name=='nt' else 'tilki-fg')
out=root/'results/modes'
out.mkdir(parents=True,exist_ok=True)

def run(name,*args,expected=0):
    directory=out/name
    directory.mkdir(exist_ok=True)
    for report in ('metrics.json','timeline.json'):
        (directory/report).unlink(missing_ok=True)
    command=[str(binary),'--config',str(out/'settings.json'),'--ui-language','en','--locale','en-US',
             '--output',str(directory),*args]
    result=subprocess.run(command,capture_output=True,text=True,timeout=120)
    assert result.returncode==expected,(name,result.stdout,result.stderr)
    assert 'Validation Error' not in result.stderr and 'SYNC-HAZARD' not in result.stderr,result.stderr
    if expected:return None
    report=directory/('timeline.json' if '--pacing-demo' in args else 'metrics.json')
    data=json.loads(report.read_text())
    assert data['real_fps'] is None and data['displayed_fps'] is None and data['input_latency_ms'] is None
    if 'cases' in data:assert data['passed'] is True
    return data

for backend in ('cpu','vulkan'):
    for mode in ('repeat','blend','interpolate','motion','optical-flow','adaptive'):
        name=backend+'-'+mode
        data=run(name,'--backend',backend,'--mode',mode,'--scene','horizontal')
        case=data['cases'][0]
        assert case['selected_mode'] in (mode,'interpolate','passthrough')
        assert case['flow_computed']==(mode not in ('repeat','blend'))
        assert data['offline_pairs_generated']==int(case['selected_mode'] in ('interpolate','blend'))
        assert data['offline_pairs_repeated']==int(case['selected_mode']=='repeat')
        if mode in ('motion','optical-flow'):
            rows=(out/name/'horizontal.flow.csv').read_text().splitlines()
            assert len(rows)==1+(256*144 if mode=='optical-flow' else 32*18)
        if mode in ('repeat','blend','motion','optical-flow'):
            reference={'repeat':'previous.ppm','blend':'blend.ppm','motion':'next.ppm','optical-flow':'next.ppm'}[mode]
            assert (out/name/'generated.ppm').read_bytes()==(out/name/reference).read_bytes()
        if mode in ('repeat','blend'):assert case['flow_computed'] is False
        if backend=='cpu':assert case['gpu_timestamp_ms'] is None,case
        elif case['gpu_timestamp_ms'] is not None:assert case['gpu_timestamp_ms']>=0,case

block=run('fractional-block','--backend','cpu','--scene','fractional','--artifact-reduction','off')
for backend in ('cpu','vulkan'):
    dense=run('fractional-'+backend,'--backend',backend,'--scene','fractional',
              '--estimator','optical-flow','--artifact-reduction','off')
    case=dense['cases'][0]
    assert case['mae_interior']<.004 and case['mae_interior']<block['cases'][0]['mae_interior']*.65,case
    assert abs(case['flow_center'][0]-5.5)<.2 and abs(case['flow_center'][1]-2.5)<.2,case

for scene in ('hud','scene_cut'):
    data=run('quality-'+scene,'--backend','vulkan','--scene',scene)
    assert data['cases'][0]['passed']
data=run('adaptive-cut','--backend','cpu','--mode','adaptive','--scene','scene_cut')
assert data['cases'][0]['decision_reason']=='scene_cut'
data=run('adaptive-display','--backend','cpu','--mode','adaptive','--display-hz','144','--scene','horizontal')
assert data['cases'][0]['selected_mode']=='passthrough' and not data['cases'][0]['flow_computed']
data=run('ideal-timeline','--mode','interpolate','--pacing-demo')
assert data['timing_basis']=='simulated' and data['simulated_source_fps']==100
assert data['simulated_submission_fps']==200 and data['simulated_interval_p99_ms']==5
assert data['simulated_generated_count']==20 and data['simulated_repeat_count']==0
repeated=run('repeated-timeline','--mode','repeat','--pacing-demo')
assert repeated['simulated_generated_count']==0 and repeated['simulated_repeat_count']==20
late=run('late-timeline','--mode','interpolate','--pacing-demo','--simulated-processing-ms','9')
assert late['simulated_dropped_count']==20 and late['simulated_submission_fps']==100
assert [e['due_ms'] for e in late['events'] if e['kind']=='real']==[e['due_ms'] for e in data['events'] if e['kind']=='real']
disabled=run('disabled-timeline','--pacing-demo','--pacing','off')
assert disabled['simulated_submission_fps']==100
for bad in ('0','nan','100oops','-2'):
    run('invalid-'+bad,'--source-fps',bad,expected=1)

# Use saved synthetic inputs to exercise real file loading; no analytic reference is claimed.
data=run('external','--backend','cpu','--mode','blend','--input-a',str(out/'cpu-repeat/previous.ppm'),
         '--input-b',str(out/'cpu-repeat/next.ppm'))
assert data['quality_reference_known'] is False and data['cases'][0]['mae_interior'] is None
assert data['cases'][0]['reference_kind']=='next_input_not_ground_truth'
weighted=run('weighted-blend','--backend','cpu','--mode','blend','--blend-weight','.25','--scene','horizontal')
assert weighted['passed'] and (out/'weighted-blend/generated.ppm').read_bytes()!=(out/'cpu-blend/generated.ppm').read_bytes()
run('bad-ppm','--backend','cpu','--input-a',str(root/'README.md'),
    '--input-b',str(out/'cpu-repeat/next.ppm'),expected=1)
print('PASS: six modes on CPU/Vulkan, fractional optical-flow improvement, analysis exports, adaptive selection, timing simulation and input failures.')
