"""Small acceptance suite for the XLISP 1.1 port."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent
checks=[
 ('arithmetic','(== (+ 7 (* 3 4)) 19)'),
 ('negative','(== (- 0 9) -9)'),
 ('division','(== (/ 21 3) 7)'),
 ('lists',"(equal (cons 1 '(2 3)) '(1 2 3))"),
 ('reverse',"(equal (reverse '(1 2 3)) '(3 2 1))"),
 ('append',"(equal (append '(1 2) '(3 4)) '(1 2 3 4))"),
 ('strings','(== (strlen (strcat "CP" "/M")) 4)'),
 ('length','(== (strlen "hello") 5)'),
 ('recursion','(== (fact 6) 720)'),
 ('arguments','(== (bump 10) 11)'),
 ('binding-restored','(== outer 99)'),
 ('loop','(== total 10)'),
 ('gc-retains-values',"(equal kept '(17 23 42))"),
 ('file-read','(== first-byte 65)'),
 ('missing-file','(null (fopen "ABSENT.TXT" "r"))'),
]
preamble='''(defun fact (n) (cond ((<= n 1) 1) (t (* n (fact (- n 1))))))
(setq outer 99)
(defun bump (outer) (+ outer 1))
(setq total 0)
(repeat 5 (setq total (+ total 2)))
(setq kept (list 17 23 42))
(repeat 300 (list 1 2 3 4))
(gc)
(princ "STRING:" (strcat "CP" "/M") "\\n")
(setq f (fopen "DATA.BIN" "w"))
(putc 65 f)
(putc 0 f)
(putc 255 f)
(fclose f)
(setq f (fopen "DATA.BIN" "r"))
(setq first-byte (getc f))
(fclose f)
'''
script=preamble+''.join('(cond (%s (print "PASS:%s")) (t (print "FAIL:%s")))\n'%(condition,name,name) for name,condition in checks)+'(exit 0)\n'
for eol in ('\n','\r\n'):
    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder);(wd/'CHECK.LSP').write_bytes(script.replace('\n',eol).encode())
        p=subprocess.run(['emu2',str(root/'xlisp.cmd'),'CHECK.LSP'],cwd=wd,
                         stdin=subprocess.DEVNULL,capture_output=True,start_new_session=True,timeout=10)
        text=(p.stdout+p.stderr).decode('latin1')
        assert 'STRING:CP/M' in text,text
        assert 'error:' not in text.lower() and 'FAIL:' not in text,text
        for name,_ in checks: assert text.count('PASS:'+name+'"')==1,(name,text)
        data=next(wd.glob('*.[bB][iI][nN]')).read_bytes()
        assert data[:3]==b'A\0\xff' and set(data[3:])<= {0x1a},data
        print('XLISP: %d checks passed (%s input)'%(len(checks)+1,'CRLF' if eol=='\r\n' else 'LF'))

with tempfile.TemporaryDirectory() as folder:
    wd=Path(folder)
    lines=[b'\n',b'A'*98+b'\n',b'B'*99+b'\n',b'C'*160+b'\n',
           b'D'*4096+b'\n',b'tail']
    payload=b''.join(lines)
    (wd/'LINES.TXT').write_bytes(payload)
    script='(setq f (fopen "LINES.TXT" "r"))\n(setq g (fopen "COPY.TXT" "w"))\n'
    for i,line in enumerate(lines):
        script+=('(setq s (fgets f))\n(gc)\n'
                 '(cond ((== (strlen s) %d) (print "LINE:%d")) '
                 '(t (print "FAIL:line")))\n(fputs s g)\n')%(len(line),i)
    script+='(cond ((null (fgets f)) (print "EOF-OK")) (t (print "FAIL:eof")))\n'
    script+='(fclose f)\n(fclose g)\n(exit 0)\n'
    (wd/'CHECK.LSP').write_text(script)
    p=subprocess.run(['emu2',str(root/'xlisp.cmd'),'CHECK.LSP'],cwd=wd,
                     stdin=subprocess.DEVNULL,capture_output=True,timeout=10)
    text=(p.stdout+p.stderr).decode('latin1')
    assert 'FAIL:' not in text and 'error:' not in text.lower(),text
    for i in range(len(lines)): assert 'LINE:%d'%i in text,text
    assert 'EOF-OK' in text,text
    data=next(p for p in wd.iterdir() if p.name.upper()=='COPY.TXT').read_bytes()
    expected=payload.replace(b'\n',b'\r\n')
    assert data[:len(expected)]==expected,(len(data),text)
    assert set(data[len(expected):]) <= {0x1a},data[len(expected):]
    print('XLISP: long-line contents, boundaries, EOF and GC retention passed')

for length,dot,reject in ((95,False,False),(96,False,True),
                          (99,True,False),(100,True,True),(160,False,True)):
    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder)
        name='A'*length
        if dot: name=name[:-1]+'.'
        pieces=' '.join('"'+name[i:i+80]+'"' for i in range(0,len(name),80))
        script='(load (strcat %s))\n(princ "NAME-CHECK-END\\n")\n(exit 0)\n'%pieces
        (wd/'CHECK.LSP').write_text(script)
        args=['CHECK.LSP'] if reject else [name]
        p=subprocess.run(['emu2',str(root/'xlisp.cmd')]+args,cwd=wd,
                         input=b'(exit 0)\r\n',capture_output=True,timeout=5)
        text=(p.stdout+p.stderr).decode('latin1')
        assert ('file name too long' in text)==reject,(length,dot,text)
        if reject:
            assert 'NAME-CHECK-END' in text,(length,dot,text)
        else:
            assert 'for input' in text,(length,dot,text)
print('XLISP: 5 filename and default-extension boundary checks passed')
