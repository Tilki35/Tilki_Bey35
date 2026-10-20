"""Compile embedded SPIR-V and a native or Windows x64 executable. No pip dependencies."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
from embed_resources import embed

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--windows', action='store_true')
args = parser.parse_args()
build = ROOT / 'build'
build.mkdir(exist_ok=True)
embed(ROOT, build)
header = ['#pragma once', '#include <cstdint>']
for name in ('flow', 'refine', 'interpolate'):
    spv = build / (name + '.spv')
    subprocess.run(['glslangValidator', '-V', '--target-env', 'vulkan1.1',
                    str(ROOT / 'shaders' / (name + '.comp')), '-o', str(spv)], check=True)
    subprocess.run(['spirv-val', '--target-env', 'vulkan1.1', str(spv)], check=True)
    data = spv.read_bytes()
    words = struct.unpack('<' + 'I' * (len(data) // 4), data)
    header.append('inline constexpr uint32_t ' + name + 'Spv[] = {' + ','.join(hex(w) for w in words) + '};')
(build / 'shaders.hpp').write_text('\n'.join(header))
# Copy only Vulkan headers so cross compilation cannot pick up Linux libc headers.
include = build / 'include'
include.mkdir(exist_ok=True)
sdk = Path(os.environ.get('VULKAN_HEADERS', '/usr/include'))
for directory in ('vulkan', 'vk_video'):
    if (sdk / directory).is_dir():
        shutil.copytree(sdk / directory, include / directory, dirs_exist_ok=True)
compiler = os.environ.get('CXX_WINDOWS', 'x86_64-w64-mingw32-g++') if args.windows else os.environ.get('CXX', 'g++')
dest = ROOT / 'dist' / ('tilki-fg.exe' if args.windows else 'tilki-fg')
dest.parent.mkdir(exist_ok=True)
command = [compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Wno-missing-field-initializers', '-DNOMINMAX',
           '-finput-charset=UTF-8', '-fexec-charset=UTF-8',
           '-I' + str(include), '-I' + str(build), '-I' + str(ROOT/'third_party'), str(ROOT / 'src/main.cpp'), '-o', str(dest)]
command += ['-static', '-static-libgcc', '-static-libstdc++'] if args.windows else ['-ldl']
if args.windows:
    command += ['-lshell32']
elif os.uname().sysname == 'Darwin':
    command = [part for part in command if part != '-ldl'] + ['-framework', 'CoreFoundation']
subprocess.run(command, check=True)
print(dest)
test_dest = build / ('core-tests.exe' if args.windows else 'core-tests')
test_command = [compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-I' + str(ROOT/'src'),
                str(ROOT/'tests/core_tests.cpp'), '-o', str(test_dest)]
test_command += ['-static', '-static-libgcc', '-static-libstdc++'] if args.windows else ['-pthread']
subprocess.run(test_command, check=True)
language_dest = build / ('language-tests.exe' if args.windows else 'language-tests')
language_command = [compiler, '-std=c++17', '-O2', '-Wall', '-Wextra',
                    '-I' + str(ROOT/'src'), '-I' + str(ROOT/'third_party'), '-I' + str(build),
                    str(ROOT/'tests/language_tests.cpp'), '-o', str(language_dest)]
language_command += ['-static', '-static-libgcc', '-static-libstdc++'] if args.windows else []
subprocess.run(language_command, check=True)
shutil.copy2(dest, dest.parent/('UniFrame.exe' if args.windows else 'uniframe'))
if args.windows:
    launcher = [compiler, '-std=c++17', '-O2', '-DNOMINMAX', '-municode', '-mwindows',
                '-I'+str(build), '-I'+str(ROOT/'src'), '-I'+str(ROOT/'third_party'),
                str(ROOT/'src/desktop_launcher_windows.cpp'), '-o', str(ROOT/'dist/EVRKare.exe'),
                '-static', '-static-libgcc', '-static-libstdc++', '-lshell32', '-luser32']
    subprocess.run(launcher, check=True)
