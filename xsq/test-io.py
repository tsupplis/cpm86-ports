"""Inject target stdio failures into the real SQ/USQ sources."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent
prefix=r'''
#include <stdio.h>
static FILE *output;
static int writes;
FILE *fault_open(name,mode)
  char *name,*mode;
{
    FILE *fp;
    fp = fopen(name,mode);
    if (*mode == 'w') output = fp;
    return fp;
}
int fault_putc(c,fp)
  int c; FILE *fp;
{
    if (fp == output && FAULT == 1 && ++writes == FAIL_AFTER)
        return EOF;
    return putc(c,fp);
}
int fault_flush(fp)
  FILE *fp;
{
    int result;
    result = fflush(fp);
    if (fp == output && FAULT == 2) return EOF;
    return result;
}
int fault_close(fp)
  FILE *fp;
{
    int result,isoutput;
    isoutput = fp == output;
    result = fclose(fp);
    if (isoutput && FAULT == 3) return EOF;
    return result;
}
fault_exit(status)
  int status;
{
    printf("EXIT-STATUS:%d\n",status);
    exit(status);
}
#undef fflush
#define fopen fault_open
#define putc fault_putc
#define fflush fault_flush
#define fclose fault_close
#define exit fault_exit
'''

for source,binary in (('xsq.c','sq'),('xusq.c','usq')):
    for mode,message,after in ((1,'write error',20),(1,'write error',1500),
                               (2,'flush error',0),(3,'close error',0)):
        with tempfile.TemporaryDirectory(prefix='xsq-io-') as name:
            work=Path(name)
            (work/'FAULT.C').write_text(prefix+(root/source).read_text())
            for command in (
                ['aztec42_cc','-I.','+F','-B','+0','-DAZTEC','-DFAULT='+str(mode),
                 '-DFAIL_AFTER='+str(after),'FAULT.C'],
                ['aztec42_link','-o','FAULT.CMD','FAULT.o','-lm','-lc86'],
            ):
                result=subprocess.run(command,cwd=work,capture_output=True,timeout=30)
                assert result.returncode==0,(command,result.stdout,result.stderr)
            payload=bytes(range(256))*8
            (work/'DATA.BIN').write_bytes(payload)
            if binary=='usq':
                result=subprocess.run(['emu2',str(root/'sq.cmd'),'DATA.BIN'],cwd=work,
                                      capture_output=True,timeout=5)
                assert result.returncode==0,result.stderr
                (work/'DATA.BIN').unlink()
            args=['DATA.BIN'] if binary=='sq' else ['-U','DATA.BQN']
            result=subprocess.run(['emu2',str(work/'FAULT.CMD')]+args,cwd=work,
                                  stdin=subprocess.DEVNULL,capture_output=True,timeout=5)
            text=(result.stdout+result.stderr).decode('latin1')
            assert message in text,(binary,mode,text)
            assert 'EXIT-STATUS:1' in text,(binary,mode,text)
            assert 'done.' not in text,(binary,mode,text)
            print('XSQ: %s injected %s reported with failure status'%(binary,message))
