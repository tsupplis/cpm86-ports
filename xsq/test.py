"""Small squeeze/unsqueeze binary integrity and checksum regression suite."""
from pathlib import Path
import subprocess
import struct
import tempfile

root=Path(__file__).resolve().parent
checks=0

def run(wd,binary,*args):
    p=subprocess.run(['emu2',str(root/(binary+'.cmd')),*args],cwd=wd,
                     stdin=subprocess.DEVNULL,capture_output=True,start_new_session=True,timeout=5)
    return (p.stdout+p.stderr).decode('latin1')

def file(wd,name):
    return next(p for p in wd.iterdir() if p.name.upper()==name.upper())

for payload in (b'',b'X'*127,b'\0'*128,bytes(range(256)),b'\x90'*4096,bytes(range(256))*16):
    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder);(wd/'DATA.BIN').write_bytes(payload)
        run(wd,'sq','DATA.BIN')
        file(wd,'DATA.BIN').unlink()
        text=run(wd,'usq','-U','DATA.BQN')
        assert 'bad checksum' not in text.lower(),text
        expected=payload+b'\0'*((-len(payload))%128)
        assert file(wd,'DATA.BIN').read_bytes()==expected,len(payload)
        checks+=1

with tempfile.TemporaryDirectory() as folder:
    wd=Path(folder);(wd/'DATA.BIN').write_bytes(bytes(range(256)))
    run(wd,'sq','DATA.BIN')
    archive=file(wd,'DATA.BQN');data=bytearray(archive.read_bytes());data[2]^=1
    archive.write_bytes(data)
    assert 'bad checksum' in run(wd,'usq','-U','DATA.BQN').lower()
    (wd/'BAD.SQ').write_bytes(b'not a squeezed file')
    assert 'not a squeezed file' in run(wd,'usq','BAD.SQ').lower()
    checks+=2

def archive(name=b'OUT.TXT', children=((-66,-257),), bits=b'\x02', crc=65):
    return (struct.pack('<HH',0xff76,crc)+name+b'\0'+
            struct.pack('<H',len(children))+
            b''.join(struct.pack('<hh',*pair) for pair in children)+bits)

bad_archives=[
 ('long filename',struct.pack('<HH',0xff76,0)+b'A'*512,'invalid original filename'),
 ('truncated filename',struct.pack('<HH',0xff76,0)+b'A'*124,'invalid original filename'),
 ('undeclared child',archive(children=((1,-257),)),'invalid decode tree'),
 ('out-of-array child',archive(children=((256,-257),)),'invalid decode tree'),
 ('invalid leaf',archive(children=((-258,-257),)),'invalid decode tree'),
 ('cycle',archive(children=((0,0),),bits=b'\0'*40),'invalid decode tree'),
 ('truncated tree',archive(children=((0,0),)*256)[:128],'invalid decode tree'),
]
for binary in ('usq','sqtype'):
    for label,payload,message in bad_archives:
        with tempfile.TemporaryDirectory() as folder:
            wd=Path(folder)
            (wd/'BAD.SQ').write_bytes(payload)
            (wd/'GOOD.SQ').write_bytes(archive(name=b'GOOD.TXT'))
            args=('-U',) if binary=='usq' else ()
            text=run(wd,binary,*args,'BAD.SQ','GOOD.SQ')
            assert message in text,(binary,label,text)
            assert 'GOOD.TXT' in text,(binary,label,text)
            if binary=='usq':
                assert file(wd,'GOOD.TXT').read_bytes()[:1]==b'A',text
                if message!='invalid decode tree' or label!='cycle':
                    assert not any(p.name.upper()=='OUT.TXT' for p in wd.iterdir()),text
            checks+=1

    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder)
        limit=311 if binary=='usq' else 127
        (wd/'BOUND.SQ').write_bytes(archive(name=b'A'*limit,crc=0,
                                          children=((-257,-257),),bits=b'\0'))
        args=('-U',) if binary=='usq' else ()
        text=run(wd,binary,*args,'BOUND.SQ')
        assert 'invalid original filename' not in text,text
        assert 'invalid decode tree' not in text,text
        checks+=1

    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder)
        children=[(i+1,i+1) for i in range(255)]+[(-257,-257)]
        (wd/'DEEP.SQ').write_bytes(archive(children=children,bits=b'\0'*32,crc=0))
        args=('-U',) if binary=='usq' else ()
        text=run(wd,binary,*args,'DEEP.SQ')
        assert 'invalid decode tree' not in text,text
        if binary=='usq':
            assert set(file(wd,'OUT.TXT').read_bytes()) <= {0},text
        checks+=1

with tempfile.TemporaryDirectory() as folder:
    wd=Path(folder)
    (wd/'BAD.SQ').write_bytes(archive(name=b'?:OUT.TXT'))
    (wd/'GOOD.SQ').write_bytes(archive(name=b'GOOD.TXT'))
    text=run(wd,'usq','-U','BAD.SQ','GOOD.SQ')
    assert "can't create ?:OUT.TXT" in text,text
    assert file(wd,'GOOD.TXT').read_bytes()[:1]==b'A',text
    checks+=1

print('XSQ: %d binary, malformed-input and recovery checks passed'%checks)
