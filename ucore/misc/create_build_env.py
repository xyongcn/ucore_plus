import os
import subprocess
import argparse


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


def call(args):
    assert subprocess.call(args) == 0


def download_file_if_not_exist(local_path, remote_url):
    if os.path.isfile(local_path):
        print(local_path + ' exists')
    else:
        call(["curl", "-o", local_path, remote_url])


def download_toolchain_src():
    print('Downloading binutils source code...')
    binutil_src_name = BINUTIL_SRC_NAME.format(BINUTIL_VERSION)
    binutil_src_url = BINUTIL_SRC_URL.format(binutil_src_name)
    download_file_if_not_exist(
        BUILD_ENV_ROOT + '/' + binutil_src_name, binutil_src_url)
    print('Downloading gcc source code...')
    for gcc_verion in GCC_VERSIONS:
        gcc_src_name = GCC_SRC_NAME.format(gcc_verion)
        gcc_src_url = GCC_SRC_URL.format(gcc_verion, gcc_src_name)
        download_file_if_not_exist(
            BUILD_ENV_ROOT + '/' + gcc_src_name, gcc_src_url)


def extract_toolchain_src():
    print('Extracting binutils source code...')
    binutil_src_name = BUILD_ENV_ROOT + '/' + \
        BINUTIL_SRC_NAME.format(BINUTIL_VERSION)
    binutil_src_dir = BUILD_ENV_ROOT + '/' + \
        BINUTIL_SRC_DIR.format(BINUTIL_VERSION)
    if os.path.isdir(binutil_src_dir):
        print(binutil_src_dir + ' exists')
    else:
        print('Extracting to' + binutil_src_dir)
        call(['tar', '-xf', binutil_src_name, '-C', BUILD_ENV_ROOT])
    print('Extracting gcc source code...')
    for gcc_verion in GCC_VERSIONS:
        gcc_src_name = BUILD_ENV_ROOT + '/' + GCC_SRC_NAME.format(gcc_verion)
        gcc_src_dir = BUILD_ENV_ROOT + '/' + GCC_SRC_DIR.format(gcc_verion)
        if os.path.isdir(gcc_src_dir):
            print(gcc_src_dir + ' exists')
        else:
            print('Extracting to' + gcc_src_dir)
            call(['tar', '-xf', gcc_src_name, '-C', BUILD_ENV_ROOT])


def reset_toolchain_build_dir():
    if os.path.isdir(TOOLCHAIN_BUILD_DIR):
        print('Deleting build directory ' + TOOLCHAIN_BUILD_DIR)
        call(['rm', '-rf', TOOLCHAIN_BUILD_DIR])
    print('Creating build directory ' + TOOLCHAIN_BUILD_DIR)
    os.makedirs(TOOLCHAIN_BUILD_DIR)


def swicth_to_toolchain_build_dir(initial_working_dir):
    os.chdir(initial_working_dir)
    reset_toolchain_build_dir()
    os.chdir(TOOLCHAIN_BUILD_DIR)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='uCore build environment setup script.'
    )
    parser.add_argument('build_dir', metavar='build_dir',
                        help='The location to setup ucore build environment.')
    parser.add_argument('--build_threads', metavar='build_threads',
                        default='4',
                        help='Number of threads used for this build.')
    parser.add_argument('--gcc_major_versions', metavar='gcc_major_versions',
                        default='7',
                        help='Number of threads used for this build.')
    args = parser.parse_args()
    gcc_major_versions = args.gcc_major_versions
    gcc_major_versions = gcc_major_versions.split(',')
    for v in gcc_major_versions:
        if v not in ['4', '5', '6', '7']:
            print("Unknown GCC major version: " + v)
            exit(1)
    GCC_VERSIONS = [v for v in GCC_VERSIONS if v[0] in gcc_major_versions]
    BUILD_ENV_ROOT = args.build_dir
    if not os.path.isdir(BUILD_ENV_ROOT):
        os.makedirs(BUILD_ENV_ROOT)
    build_thread_param = '-j' + args.build_threads
    TOOLCHAIN_BUILD_DIR = args.build_dir + '/' + 'build'
    print('Setting up uCore build environment on ' + BUILD_ENV_ROOT)
    initial_working_dir = os.getcwd()
    download_toolchain_src()
    extract_toolchain_src()

    for ucore_arch in ARCHITECTURE_MAP:
        gcc_arch = ARCHITECTURE_MAP[ucore_arch]
        for gcc_version in GCC_VERSIONS:
            swicth_to_toolchain_build_dir(initial_working_dir)
            binutil_build_configure = BUILD_ENV_ROOT + '/' \
                + BINUTIL_SRC_DIR.format(BINUTIL_VERSION) + '/' + 'configure'
            gcc_build_configure = BUILD_ENV_ROOT + '/' + \
                GCC_SRC_DIR.format(gcc_version) + '/' + 'configure'
            build_env_path = BUILD_ENV_ROOT + '/' + \
                'env-' + ucore_arch + '-gcc-' + gcc_version
            if os.path.isdir(build_env_path):
                print(build_env_path + ' exists.')
                continue
            call([
                binutil_build_configure,
                '--prefix=' + build_env_path,
                '--target=' + gcc_arch,
                '--disable-multilib',
            ])
            call(['make', build_thread_param])
            call(['make', 'install'])
            swicth_to_toolchain_build_dir(initial_working_dir)
            call([
                gcc_build_configure,
                '--prefix=' + build_env_path,
                '--target=' + gcc_arch,
                '--disable-multilib',
                '--enable-languages=c',
                '--without-headers',
            ])
            call(['make', build_thread_param, 'all-gcc'])
            call(['make', 'install-gcc'])
            if ucore_arch == 'arm':
                call(['make', build_thread_param, 'all-target-libgcc'])
                call(['make', 'install-target-libgcc'])

    os.chdir(initial_working_dir)
    call(['rm', '-rf', TOOLCHAIN_BUILD_DIR])
