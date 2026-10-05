"""Audit the actual native BAR, assets, digests, identity, icon and ELF."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile
ROOT=Path(__file__).resolve().parents[1]

def audit(path):
    path=Path(path)
    with zipfile.ZipFile(path) as bar:
        names=bar.namelist()
        if len(names)!=len(set(names)) or any(n.startswith('/') or '..' in n.split('/') or '\\' in n for n in names):
            raise ValueError('Unsafe/duplicate ZIP asset')
        manifest=re.sub(r'\r?\n ','',bar.read('META-INF/MANIFEST.MF').decode('utf-8'))
        sections=re.split(r'\r?\n\r?\n',manifest)
        main={}
        for section in sections:
            fields=dict(line.split(': ',1) for line in section.splitlines() if ': ' in line)
            if 'Archive-Asset-Name' not in fields:
                main.update(fields)
        package=main.get('Package-Name','');diagnostic=package=='top.blaccat.BRCodeTests'
        if package not in ('top.blaccat.BRCode','top.blaccat.BRCodeTests'):raise ValueError('Wrong package identity: '+package)
        inventory=[];declared=set()
        for section in sections[1:]:
            fields=dict(line.split(': ',1) for line in section.splitlines() if ': ' in line)
            name=fields.get('Archive-Asset-Name')
            if name is None:continue
            if name in declared:raise ValueError('Duplicate Manifest asset: '+name)
            data=bar.read(name)
            actual=base64.urlsafe_b64encode(hashlib.sha512(data).digest()).decode().rstrip('=')
            if actual!=fields['Archive-Asset-SHA-512-Digest'].rstrip('='):raise ValueError('SHA-512 mismatch: '+name)
            declared.add(name);inventory.append(dict(path=name,bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
        assets={n for n in names if not n.startswith('META-INF/') and not n.endswith('/')}
        if assets!=declared:raise ValueError('Manifest/ZIP asset set mismatch')
        entry='native/brcode-tests' if diagnostic else 'native/brcode';elf=bar.read(entry)
        if elf[:7]!=b'\x7fELF\x01\x01\x01' or struct.unpack_from('<H',elf,18)[0]!=40 or struct.unpack_from('<H',elf,16)[0] not in (2,3):
            raise ValueError('Entry is not executable little-endian ELF32 ARM')
        if b'libstdc++' in elf or b'libcpp.so.4' not in elf:raise ValueError('Wrong C++ ABI')
        if main.get('Entry-Point-Type')!='Qnx/Elf' or main.get('Entry-Point','').split()[-1:]!=['app/'+entry]:raise ValueError('Invalid native entry')
        if set(main.get('Entry-Point-User-Actions','').split(','))!={'use_camera','access_shared'}:raise ValueError('Unexpected application permissions')
        icon=bar.read('native/icon.png')
        if icon[:8]!=b'\x89PNG\r\n\x1a\n' or icon[12:16]!=b'IHDR' or struct.unpack_from('>II',icon,16)!=(114,114):raise ValueError('Invalid Q10 PNG icon')
        required=['assets/main.qml','assets/SystemTextArea.qml','assets/SystemTextField.qml']
        for name in required:
            if not bar.read('native/'+name):raise ValueError('Required asset missing/empty: '+name)
        if any('/ime/' in n or 'bbime' in n.lower() or 'pinyin' in n.lower() for n in assets):
            raise ValueError('Legacy input module packaged in the system-input build')
        descriptor=ET.parse(ROOT/('bar-descriptor-tests.xml' if diagnostic else 'bar-descriptor.xml')).getroot()
        for asset in descriptor.findall('{*}asset'):
            source=ROOT/asset.attrib['path'];destination='native/'+asset.text
            paths=[(p,destination+'/'+p.relative_to(source).as_posix()) for p in source.rglob('*') if p.is_file()] if source.is_dir() else [(source,destination)]
            for local,packaged in paths:
                if local.read_bytes()!=bar.read(packaged):raise ValueError('BAR differs from current source: '+packaged)
        version=descriptor.find('{*}versionNumber').text+'.'+descriptor.find('{*}buildId').text
        if main.get('Package-Version')!=version:raise ValueError('Unexpected application version')
        return dict(path=str(path.resolve()),packageName=package,version=main['Package-Version'],
                    sha256=hashlib.sha256(path.read_bytes()).hexdigest(),diagnostic=diagnostic,
                    entry=entry,elf='ELF32 little-endian ARM libcpp.so.4',inputMethod='system',manifest=main,assets=inventory,status='PASS')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('bar',type=Path);parser.add_argument('--output',type=Path);args=parser.parse_args()
    result=audit(args.bar)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k not in ('manifest','assets')},ensure_ascii=False))
