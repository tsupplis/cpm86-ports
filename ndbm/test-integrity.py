import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parent
for ext in ('cmd','com'):
    with tempfile.TemporaryDirectory() as folder:
        wd=pathlib.Path(folder)
        (wd/'BINARY.pag').touch()
        (wd/'BINARY.dir').touch()
        def run(*args):
            p=subprocess.run(['emu2',str(root/'tests'/('testdb.'+ext)),*args],cwd=wd,
                             stdin=subprocess.DEVNULL,capture_output=True,start_new_session=True,timeout=30)
            text=(p.stdout+p.stderr).decode('latin1')
            assert 'FAIL:' not in text, text
            return text
        assert 'INTEGRITY PASS' in run()
        pagefile=next(wd.glob('*.[pP][aA][gG]'))
        assert pagefile.stat().st_size>1024, 'multi-page case was not exercised'
        print(ext+': multi-page binary persistence, replacement, deletion, iteration, and failed writes PASS')
