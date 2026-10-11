import frida, subprocess, time, sys, datetime

ADB = ['adb', '-s', '2G0YC5ZF9Q01J0', 'shell']
dev = frida.get_device_manager().add_remote_device('127.0.0.1:27042')
def P(s): print(datetime.datetime.now().strftime("%H:%M:%S ") + s, flush=True)

pid = int(subprocess.run(ADB + ['pidof', 'VirtualDesktop.Android'], capture_output=True, text=True).stdout.split()[0])
P("VD pid=%d" % pid)
s = dev.attach(pid)
code = open(r"C:\Users\remus\AppData\Local\Temp\opencode\inject.js").read()
script = s.create_script(code)
script.on('message', lambda m, d: P("MSG " + (str(m.get('payload')) if m.get('type') == 'send' else str(m))))
script.load()
P("injector loaded; open Virtual Desktop and look behind the desktop for the IR feed")

dur = int(sys.argv[1]) if len(sys.argv) > 1 else 90
t0 = time.time()
while time.time() - t0 < dur:
    time.sleep(5)
    try:
        st = script.exports_sync.stats()
        P("stats %s" % (st,))
    except Exception as e:
        P("stats err %s" % e)
P("done")
