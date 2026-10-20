"""Compare full retained original/production COM timelines and exact PCM uploads.
The only permitted difference is explicit safe resource release during final
close (original40a650 sets availability0 first and leaks cached references).
"""
import pathlib,hashlib,json
import pefile
import original_resources
root=pathlib.Path(__file__).resolve().parents[2];directory=root/'research/sniper87/runtime-audio'
pe=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'))
resource_paths=[]
for kind in pe.DIRECTORY_ENTRY_RESOURCE.entries:
 if str(kind.name)!='WAVE':continue
 for entry in kind.directory.entries:
  for language in entry.directory.entries:
   info=language.data.struct;key=f'WAVE-{entry.id}-{language.id}'
   assert original_resources.payloads()[key]==pe.get_data(info.OffsetToData,info.Size),key
   resource_paths.append(key)
assert len(resource_paths)==17
results=[];total=0;bytes_checked=0;buffers_checked=0;safe_releases=0
for scenario in range(6):
 a=(directory/f'original-adapter-trace-{scenario}.txt').read_text().splitlines()
 b=(directory/f'production-adapter-trace-{scenario}.txt').read_text().splitlines()
 if scenario==5:assert a==b==['device_create_fail'];extra=[]
 else:
  assert a[-1]==b[-1]=='release_device'
  prefix=a[:-1];assert b[:len(prefix)]==prefix, next(((i,x,y) for i,(x,y) in enumerate(zip(prefix,b)) if x!=y),('length',len(prefix),len(b)))
  # All objects before MARK events are cached resource buffers. Dynamic voice
  # creation later is separately released in the matching trace prefix.
  boundary=a.index('MARK events');ids=[]
  for line in a[:boundary]:
   parts=line.split()
   if parts[0]=='create':ids.append(int(parts[1]))
   if parts[0]=='duplicate':ids.append(int(parts[2]))
  extra=b[len(prefix):-1];assert extra==[f'release {i}' for i in ids],(scenario,extra,ids)
 uploads=sum(line.startswith('unlock ') for line in a)
 for i in range(uploads):
  x=(directory/f'original-buffer-{scenario}-{i}.bin').read_bytes();y=(directory/f'production-buffer-{scenario}-{i}.bin').read_bytes();assert x==y,(scenario,i)
  buffers_checked+=1;bytes_checked+=len(x)
 total+=len(a);safe_releases+=len(extra);results.append({'scenario':scenario,'original_events':len(a),'production_events':len(b),'uploads':uploads,'documented_extra_safe_releases':len(extra)})
paths=[root/'code/build/assets/files/SNIPER.EXE',root/'code/tests/audio_adapter_trace.c',root/'code/tests/audio_adapter_native_trace.py',root/'code/tests/audio_adapter_compare.py',root/'code/platform/audio_player.c',root/'code/platform/audio_player.h',root/'code/src/audio_logic.c',root/'code/src/audio_logic.h',directory/'adapter-trace.exe']
paths+=sorted(directory.glob('original-adapter-trace-*.txt'))+sorted(directory.glob('production-adapter-trace-*.txt'))+sorted(directory.glob('original-buffer-*.bin'))+sorted(directory.glob('production-buffer-*.bin'))
manifest={'result':'PASS with documented cleanup divergence','scenarios':results,'original_events':total,'exact_pcm_uploads':buffers_checked,'exact_pcm_bytes':bytes_checked,'documented_extra_safe_releases':safe_releases,'sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
manifest['sha256'].update({f'SNIPER.EXE resource {k}':hashlib.sha256(original_resources.payloads()[k]).hexdigest() for k in resource_paths})
(directory/'audio-adapter-verification.json').write_text(json.dumps(manifest,indent=2)+'\n')
message=f'PASS6 complete original/production audio timelines:{total} original COM/phase events,{buffers_checked} byte-identical PCM uploads/{bytes_checked} bytes;only documented cleanup difference:{safe_releases} additional cached-buffer Releases before device Release'
(directory/'audio-adapter-result.txt').write_text(message+'\n');print(message)
