"""Compile and execute the real portable HistoryStore/ScanGate, not replicas."""
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import sqlite3
import subprocess
import sys
import uuid
import hashlib
import urllib.request
import zipfile
import io
ROOT=Path(__file__).resolve().parents[1]

def main():
    sys.stdout.reconfigure(encoding='utf-8')
    compiler=shutil.which('g++')
    if not compiler:raise RuntimeError('Host C++98 compiler unavailable')
    output=ROOT/'build/host';output.mkdir(parents=True,exist_ok=True)
    include=output/'include';include.mkdir(exist_ok=True)
    sdk=Path(os.environ.get('BRCODE_SDK_ROOT') or os.environ.get('INTROOP_SDK_ROOT') or 'C:/bbdevtools')
    shutil.copyfile(sdk/'target_10_3_1_995/qnx6/usr/include/sqlite3.h',include/'sqlite3.h')
    executable=output/'host-tests.exe'
    machine=subprocess.check_output([compiler,'-dumpmachine'],text=True).strip()
    vendor=output/'sqlite';vendor.mkdir(exist_ok=True)
    amalgamation=vendor/'sqlite3.c'
    expected='62aaecaacab3a4bf4a8fe4aec1cfdc1571fe9a44'
    if not amalgamation.is_file():
        data=urllib.request.urlopen('https://www.sqlite.org/sqlite-amalgamation-3071401.zip',timeout=30).read()
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            name=next(n for n in archive.namelist() if n.endswith('/sqlite3.c'))
            source=archive.read(name)
            if hashlib.sha1(source).hexdigest()!=expected:raise ValueError('Historical SQLite source identity mismatch')
            amalgamation.write_bytes(source)
    if hashlib.sha1(amalgamation.read_bytes()).hexdigest()!=expected:raise ValueError('SQLite source drift')
    sqlite_object=output/'sqlite3.o'
    c_compiler=Path(compiler).with_name('gcc.exe')
    compiled=subprocess.run([str(c_compiler),'-O2','-DSQLITE_THREADSAFE=0','-c',str(amalgamation),'-o',str(sqlite_object)],capture_output=True)
    (output/'sqlite-compile.stderr.log').write_bytes(compiled.stderr)
    if compiled.returncode:raise RuntimeError(compiled.stderr.decode('utf-8',errors='replace'))
    link=[str(sqlite_object),'-static','-static-libstdc++','-static-libgcc']
    arguments=[compiler,'-std=c++98','-Wall','-Wextra','-Isrc','-I'+str(include),
               'tests/HostTests.cpp','src/HistoryStore.cpp',*link,'-o',str(executable)]
    built=subprocess.run(arguments,cwd=ROOT,capture_output=True)
    (output/'compile.stdout.log').write_bytes(built.stdout);(output/'compile.stderr.log').write_bytes(built.stderr)
    if built.returncode:raise RuntimeError(built.stderr.decode('utf-8',errors='replace'))
    run=output/('run-'+uuid.uuid4().hex);run.mkdir()
    compiler_root=Path(compiler).parent.parent
    runtime=compiler_root/'usr'/machine/'sys-root/mingw/bin'
    env=dict(os.environ);env['PATH']=';'.join(map(str,[runtime,Path(compiler).parent]))+';'+env['PATH']
    result=subprocess.run([str(executable)],cwd=run,env=env,capture_output=True)
    (run/'stdout.log').write_bytes(result.stdout);(run/'stderr.log').write_bytes(result.stderr)
    if result.returncode:raise RuntimeError('Host test exit '+str(result.returncode)+': '+result.stderr.decode('utf-8',errors='replace'))
    summary=json.loads(re.search(rb'RESULT (.+)',result.stdout).group(1))
    summary.update(executedAt=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                   scope='actual C++98 HistoryStore / sqlite3, ScanGate',compiler=compiler,
                   evidenceDirectory=str(run),sqliteVersion='3.7.14.1',sqliteSourceSha1=expected,nativeQr='NOT TESTED',camera='NOT TESTED')
    (output/'results.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(summary,ensure_ascii=False))

if __name__=='__main__':main()
