"""Validate descriptor sources and the system-input build boundary."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]

def verify():
    for descriptor in ['bar-descriptor.xml','bar-descriptor-tests.xml']:
        for asset in ET.parse(ROOT/descriptor).getroot().findall('{*}asset'):
            if 'ime/' in asset.attrib['path'] or 'Ime' in asset.attrib['path']:
                raise ValueError('Legacy input asset declared: '+asset.attrib['path'])
            path=ROOT/asset.attrib['path']
            if not path.exists() and not asset.attrib['path'].startswith('build/'):
                raise ValueError('Missing descriptor asset: '+str(path))
    sources=[ROOT/'BRCode.pro',ROOT/'build.ps1']+list((ROOT/'src').glob('*'))+list((ROOT/'assets').glob('*.qml'))
    for path in sources:
        if not path.is_file():continue
        content=path.read_text(encoding='utf-8-sig')
        if any(token in content for token in ['bbime::','src/ime/','import "ime"','InputMode.Custom',
                                              'VirtualKeyboardOff','ime.ready','ime.enabled','routeKey(',
                                              'NativeSymbolPanel','CandidateStrip','ImeToggle']) or re.search(r'\bime\s*:',content):
            raise ValueError('Custom input integration remains: '+str(path))
    print('PASS: declared source assets and system-input build boundary')
if __name__=='__main__':verify()
