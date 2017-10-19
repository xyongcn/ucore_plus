import sys
import os
import subprocess


BINUTIL_VERSION = '2.29.1'
GCC_VERSIONS = ['4.9.4', '5.5.0', '6.4.0', '7.2.0']
BINUTIL_SRC_NAME = 'binutils-{}.tar.gz'
BINUTIL_SRC_DIR = 'binutils-{}'
BINUTIL_SRC_URL = 'https://ftp.gnu.org/gnu/binutils/{}'
GCC_SRC_NAME = 'gcc-{}.tar.gz'
GCC_SRC_DIR = 'gcc-{}'
GCC_SRC_URL = 'https://ftp.gnu.org/gnu/gcc/gcc-{}/{}'
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


def call(args):
    assert subprocess.call(args) == 0


def evaluate_build(ucore_arch, board, compiler_version):
    os.environ['CROSS_COMPILE'] = '/home/tinytangent/ucore_build_environment/' + \
        'env-' + ucore_arch + '-gcc-' + compiler_version + '/bin/' + ARCHITECTURE_MAP[ucore_arch] + '-'
    subprocess.call(['make', 'clean'])
    call(['make', 'ARCH=' + ucore_arch, 'BOARD=' + board, 'defconfig'])
    if ucore_arch != 'arm':
        call(['make'])
        call(['make', 'sfsimg'])
        # subprocess.call(['./uCore_run'])
    else:
        call(['make', 'sfsimg'])
        call(['make'])
    # result = input("Report the running result: ")
    # print(result)
    subprocess.call(['make', 'clean'])


if __name__ == '__main__':
    ALL_ARCH = ['i386', 'amd64', 'arm']
    test_result = []
    all_test_passed = True
    for ucore_arch in ALL_ARCH:
        for board in EXISTING_BOARDS[ucore_arch]:
            for compiler_version in GCC_VERSIONS:
                if compiler_version[0] == '4' and ucore_arch == 'arm':
                    continue
                test_name = ucore_arch + '_' + board + '_' + compiler_version
                compile_result = 'pass'
                try:
                    evaluate_build(ucore_arch, board, compiler_version)
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
