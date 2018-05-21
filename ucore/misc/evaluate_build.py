import sys
import os
import subprocess
import argparse


BINUTIL_VERSION = '2.29.1'
GCC_VERSIONS = ['4.9.4', '5.5.0', '6.4.0', '7.2.0']
BUILD_ENV_ROOT = ''
TOOLCHAIN_BUILD_DIR = ''
ARCHITECTURE_MAP = {
    'i386': 'i386-linux-gnu',
    'amd64': 'x86_64-linux-gnu',
    'mips': 'mips-sde-elf',
    'arm': 'arm-none-eabi',
}
EXISTING_BOARDS = {
    'i386': ['default'],
    'amd64': ['default'],
    'arm': ['goldfishv7', 'pandaboardes', 'raspberrypi', 'zedboard'],
    'mips': ['default']
}


def evaluate_build(build_env_dir, ucore_arch, board, compiler_version):
    try:
        from subprocess import DEVNULL  # Python 3.
    except ImportError:
        DEVNULL = open(os.devnull, 'wb')
    os.environ['CROSS_COMPILE'] = build_env_dir + '/' + \
        'env-' + ucore_arch + '-gcc-' + compiler_version + '/bin/' + ARCHITECTURE_MAP[ucore_arch] + '-'
    subprocess.call(['make', 'clean'])
    subprocess.check_call(['make', 'ARCH=' + ucore_arch, 'BOARD=' + board, 'defconfig'])
    if ucore_arch != 'arm' and ucore_arch != 'mips':
        subprocess.check_call(['make'], stdout=DEVNULL, stderr=subprocess.STDOUT)
        subprocess.check_call(['make', 'sfsimg'], stdout=DEVNULL, stderr=subprocess.STDOUT)
        # subprocess.call(['./uCore_run'])
    else:
        subprocess.check_call(['make', 'sfsimg'], stdout=DEVNULL, stderr=subprocess.STDOUT)
        subprocess.check_call(['make'], stdout=DEVNULL, stderr=subprocess.STDOUT)
    # result = input("Report the running result: ")
    # print(result)
    subprocess.call(['make', 'clean'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='uCore build environment setup script.'
    )
    parser.add_argument('build_env_dir', metavar='build_env_dir',
                        help='The location of ucore build environment.')
    parser.add_argument('--gcc_major_versions', metavar='gcc_major_versions',
                        default='4,5,6,7',
                        help='GCC major version for build check.')
    parser.add_argument('--ucore_arch', metavar='ucore_arch',
                        default='i386,amd64,arm',
                        help='Specify uCore architecture.')
    parser.add_argument('--ucore_board', metavar='ucore_arch',
                        default='all',
                        help='Specify uCore board.')
    parser.add_argument('--print_detail', metavar='ucore_arch',
                        help='Specify uCore board.')
    args = parser.parse_args()
    gcc_major_versions = args.gcc_major_versions
    gcc_major_versions = gcc_major_versions.split(',')
    for v in gcc_major_versions:
        if v not in ['4', '5', '6', '7']:
            print("Unknown GCC major version: " + v)
            exit(1)
    gcc_major_versions = [v for v in GCC_VERSIONS if v[0] in gcc_major_versions]
    all_arch = args.ucore_arch.split(',')
    build_env_dir = args.build_env_dir
    test_result = []
    all_test_passed = True
    for ucore_arch in all_arch:
        if args.ucore_board != 'all':
            all_boards = args.ucore_board.split(',')
        else:
            all_boards = EXISTING_BOARDS[ucore_arch]
        for board in all_boards:
            for compiler_version in gcc_major_versions:
                if compiler_version[0] == '4' and ucore_arch == 'arm':
                    continue
                test_name = ucore_arch + '_' + board + '_' + compiler_version
                compile_result = 'pass'
                try:
                    evaluate_build(build_env_dir, ucore_arch, board, compiler_version)
                except AssertionError:
                    compile_result = 'FAIL'
                    all_test_passed = False
                test_result.append([test_name, compile_result])
    for entry in test_result:
        print(entry[0] + ':' + ' ' + 'compile->' + entry[1])
    if all_test_passed:
        sys.exit(0)
    else:
        sys.exit(1)
