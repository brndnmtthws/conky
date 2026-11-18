#!/usr/bin/env python3
"""Exercise the real extras build/install rules without Conky's native dependencies.

Run with: python tests/test-extras-install.py /path/to/cmake /path/to/conky
Requires the same Jinja2/PyYAML modules as the extras renderer.
"""

import os
from pathlib import Path
import subprocess
import sys
import tempfile

cmake = sys.argv[1]
source = Path(sys.argv[2]).resolve()


def run(*args, **kwargs):
    subprocess.run(args, check=True, **kwargs)


def check_install(root, name, options, data_dir, vim_dir, install_prefix=None):
    build = root / name
    stage = root / (name + "-stage")
    run(cmake, "-S", str(root / "fixture"), "-B", str(build),
        "-DCONKY_SOURCE_DIR=" + str(source),
        "-DPython3_EXECUTABLE=" + sys.executable, *options)
    run(cmake, "--build", str(build))
    install_args = [cmake, "--install", str(build)]
    if install_prefix:
        install_args += ["--prefix", install_prefix]
    run(*install_args, env={**os.environ, "DESTDIR": str(stage)})

    expected = {}
    if data_dir:
        expected[data_dir + "/nano/conky.nanorc"] = build / "extras/nano/conky.nanorc"
    if vim_dir:
        expected[vim_dir + "/syntax/conkyrc.vim"] = build / "extras/vim/syntax/conkyrc.vim"
        expected[vim_dir + "/ftdetect/conkyrc.vim"] = source / "extras/vim/ftdetect/conkyrc.vim"
    actual = {"/" + str(p.relative_to(stage)) for p in stage.rglob("*") if p.is_file()}
    assert actual == set(expected), (name, actual, set(expected))
    for path, original in expected.items():
        contents = (stage / path.lstrip("/")).read_bytes()
        assert contents and contents == original.read_bytes(), (name, path)
    print("PASS:", name, flush=True)


with tempfile.TemporaryDirectory(prefix="conky-extras-") as temporary:
    root = Path(temporary)
    fixture = root / "fixture"
    fixture.mkdir()
    # Expose the real renderer at the source-root path expected by extras.
    (fixture / "doc").symlink_to(source / "doc", target_is_directory=True)
    (fixture / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.17)
project(extras_install LANGUAGES C)
option(BUILD_EXTRAS "Build extras" ON)
if(BUILD_EXTRAS)
  add_subdirectory("${CONKY_SOURCE_DIR}/extras" extras)
endif()
''')
    check_install(root, "default", [], "/usr/local/share", "/usr/local/share/vim")
    check_install(root, "system", ["-DCMAKE_INSTALL_PREFIX=/usr"],
                  "/usr/share", "/usr/share/vim")
    check_install(root, "dataroot", ["-DCMAKE_INSTALL_DATAROOTDIR=custom data"],
                  "/usr/local/custom data", "/usr/local/custom data/vim")
    check_install(root, "relative-runtime", ["-DVIM_RUNTIME_DIR=share/vim/vimfiles"],
                  "/opt/conky/share", "/opt/conky/share/vim/vimfiles", "/opt/conky")
    check_install(root, "absolute-runtime", ["-DVIM_RUNTIME_DIR=/opt/vim files"],
                  "/usr/local/share", "/opt/vim files")
    check_install(root, "absolute-dataroot", ["-DCMAKE_INSTALL_DATAROOTDIR=/opt/data"],
                  "/opt/data", "/opt/data/vim")
    check_install(root, "disabled-vim", ["-DVIM_RUNTIME_DIR="],
                  "/usr/local/share", None)
    check_install(root, "disabled-extras", ["-DBUILD_EXTRAS=OFF"], None, None)
