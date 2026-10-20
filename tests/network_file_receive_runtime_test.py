"""Win32 file sink exercised through recovered opcode18 decoder, no network."""
import pathlib,subprocess,os,json,zlib
root=pathlib.Path(__file__).resolve().parents[2];run=root/'research/sniper87/runtime-file-receive';run.mkdir(exist_ok=True)
exe=root/'code/platform/build/network-file-receive-test.exe'
subprocess.run(['i686-w64-mingw32-gcc','-std=c11','-Wall','-Wextra','-Werror',str(root/'code/tests/network_file_receive_runtime_test.c'),str(root/'code/platform/network_file_receive.c'),str(root/'code/src/network_game_receive.c'),'-o',str(exe)],check=True)
env=os.environ.copy();env.update(WINEPREFIX=str(run/'wine'),WINEDEBUG='-all')
try:result=subprocess.run(['xvfb-run','-a','wine',str(exe)],cwd=run,env=env,capture_output=True,text=True,timeout=100)
finally:subprocess.run(['wineserver','-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
(run/'result.txt').write_text(result.stdout);(run/'wine.log').write_text(result.stderr)
assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
files=run/'files';metadata=json.loads((files/'fixture.txt.source.json').read_text());assert metadata['hostname']=='127.0.0.1' and metadata['tcp_port']=='382' and metadata['bytes']==9 and metadata['actual_checksum']=='cbf43926'
assert metadata['checksum_algorithm']=='CRC-32/ISO-HDLC' and metadata['expected_checksum'] is None
backup=[p for p in files.glob('old*.tmp') if p.read_bytes()==b'previous bytes'];assert backup and any(pathlib.Path(str(p)+'.source.json').read_text()=='previous provenance' for p in backup)
meta2=json.loads((files/'decoder.bin.source.json').read_text());assert meta2['actual_checksum']==f'{zlib.crc32(b"bytes"):08x}'
assert not list(files.glob('s87*.tmp')) and not list(files.glob('met*.tmp'))
print(result.stdout.strip());print('PASS sidecar content, independent CRC, paired backup and no orphan stages')
