#!/usr/bin/env python3
"""Check source volume and default switching using an isolated PulseAudio server.

Run with the Conky executable as the sole argument. Requires pulseaudio and
pactl in PATH; does not connect to or modify the user's audio server.
"""

import os
from pathlib import Path
import selectors
import shutil
import subprocess
import sys
import tempfile
import time


def main():
    if not shutil.which("pulseaudio") or not shutil.which("pactl"):
        print("SKIP: pulseaudio and pactl are required")
        return 77
    conky = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="conky-pulse-") as directory:
        root = Path(directory)
        env = os.environ.copy()
        env.update(
            HOME=directory,
            XDG_CONFIG_HOME=directory,
            XDG_RUNTIME_DIR=directory,
            PULSE_RUNTIME_PATH=directory,
            PULSE_STATE_PATH=directory,
            PULSE_COOKIE=str(root / "cookie"),
            PULSE_SERVER=f"unix:{root / 'native'}",
        )
        (root / "cookie").write_bytes(os.urandom(256))
        server_config = root / "server.pa"
        server_config.write_text(
            f"load-module module-native-protocol-unix socket={root / 'native'} "
            f"auth-cookie={root / 'cookie'}\n"
            "load-module module-null-source source_name=conky_source_a\n"
            "load-module module-null-source source_name=conky_source_b\n"
            "set-default-source conky_source_a\n"
        )
        config = root / "conky.conf"
        config.write_text(
            "conky.config = {out_to_console=true, out_to_x=false, "
            "background=false, update_interval=0.1, total_run_times=200};\n"
            "conky.text = [[source=${pa_source_volume}:"
            "${if_pa_source_muted}muted${else}unmuted${endif}]];\n"
        )
        processes = []
        with (root / "server.log").open("w+") as server_log, (
            root / "conky.log"
        ).open("w+") as conky_log:
            try:
                server = subprocess.Popen(
                    [
                        "pulseaudio", "-n", "--daemonize=no",
                        "--use-pid-file=no", "--exit-idle-time=-1",
                        "--disable-shm=yes", "--log-target=stderr",
                        "-F", str(server_config),
                    ],
                    env=env, stdout=server_log, stderr=server_log,
                )
                processes.append(server)

                def pactl(*args):
                    return subprocess.run(
                        ["pactl", *args], env=env, capture_output=True,
                        text=True, timeout=3, check=True,
                    ).stdout

                deadline = time.monotonic() + 5
                while True:
                    try:
                        pactl("info")
                        break
                    except subprocess.CalledProcessError:
                        if server.poll() is not None or time.monotonic() > deadline:
                            raise RuntimeError("isolated PulseAudio did not start")
                        time.sleep(0.05)

                pactl("set-source-volume", "conky_source_a", "25%")
                pactl("set-source-volume", "conky_source_b", "75%")
                client = subprocess.Popen(
                    [conky, "-c", str(config)], env=env,
                    stdout=subprocess.PIPE, stderr=conky_log,
                )
                processes.append(client)
                selector = selectors.DefaultSelector()
                selector.register(client.stdout, selectors.EVENT_READ)
                received = b""

                def expect(value):
                    nonlocal received
                    deadline = time.monotonic() + 5
                    wanted = f"source={value}".encode()
                    while time.monotonic() < deadline:
                        for key, _ in selector.select(timeout=0.1):
                            data = os.read(key.fileobj.fileno(), 4096)
                            if not data:
                                raise RuntimeError("Conky exited before expected output")
                            received += data
                            if wanted in received:
                                received = b""
                                print(f"PASS: {wanted.decode()}")
                                return
                    raise AssertionError(f"missing {wanted!r}; got {received!r}")

                expect("25:unmuted")
                pactl("set-source-volume", "conky_source_a", "40%")
                expect("40:unmuted")
                pactl("set-source-mute", "conky_source_a", "1")
                expect("40:muted")
                pactl("set-default-source", "conky_source_b")
                expect("75:unmuted")
                pactl("set-source-volume", "conky_source_a", "10%")
                pactl("set-source-volume", "conky_source_b", "60%")
                expect("60:unmuted")
                pactl("set-default-source", "conky_source_a")
                expect("10:muted")
                selector.close()
            except Exception:
                server_log.seek(0)
                conky_log.seek(0)
                print(server_log.read(), file=sys.stderr)
                print(conky_log.read(), file=sys.stderr)
                raise
            finally:
                for process in reversed(processes):
                    if process.poll() is None:
                        process.terminate()
                    try:
                        process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=3)
    return 0


if __name__ == "__main__":
    sys.exit(main())
