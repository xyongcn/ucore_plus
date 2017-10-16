import os
from subprocess import call


BINUTIL_VERSION = '2.29.1'
GCC_VERSIONS = ['4.9.4', '5.5.0', '6.4.0', '7.2.0']
BINUTIL_SRC_NAME = 'binutils-{}.tar.gz'
BINUTIL_SRC_DIR = 'binutils-{}'
BINUTIL_SRC_URL = 'https://ftp.gnu.org/gnu/binutils/{}'
GCC_SRC_NAME = 'gcc-{}.tar.gz'
GCC_SRC_DIR = 'gcc-{}'
GCC_SRC_URL = 'https://ftp.gnu.org/gnu/gcc/gcc-{}/{}'
BUILD_ENV_ROOT = '/home/tinytangent/ucore_build_environment'
TOOLCHAIN_BUILD_DIR = BUILD_ENV_ROOT + '/' + 'build'
ARCHITECTURE_MAP = {
    'i386': 'i386-linux-gnu',
    'amd64': 'x86_64-linux-gnu',
    'mips': 'mips-sde-elf',
    'arm': 'arm-none-eabi',
}

def download_file_if_not_exist(local_path, remote_url):
    if os.path.isfile(local_path):
        print(local_path + ' exists')
    else:
        call(["curl", "-o", local_path, remote_url])


def download_toolchain_src():
    print('Downloading binutils source code...')
    binutil_src_name = BINUTIL_SRC_NAME.format(BINUTIL_VERSION)
    binutil_src_url = BINUTIL_SRC_URL.format(binutil_src_name)
    download_file_if_not_exist(BUILD_ENV_ROOT + '/' + binutil_src_name, binutil_src_url)
    print('Downloading gcc source code...')
    for gcc_verion in GCC_VERSIONS:
        gcc_src_name = GCC_SRC_NAME.format(gcc_verion)
        gcc_src_url = GCC_SRC_URL.format(gcc_verion, gcc_src_name)
        download_file_if_not_exist(BUILD_ENV_ROOT + '/' + gcc_src_name, gcc_src_url)


def extract_toolchain_src():
    print('Extracting binutils source code...')
    binutil_src_name = BUILD_ENV_ROOT + '/' + BINUTIL_SRC_NAME.format(BINUTIL_VERSION)
    binutil_src_dir = BUILD_ENV_ROOT + '/' + BINUTIL_SRC_DIR.format(BINUTIL_VERSION)
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


initial_working_dir = os.getcwd()
download_toolchain_src()
extract_toolchain_src()

for gcc_version in GCC_VERSIONS:
    swicth_to_toolchain_build_dir(initial_working_dir)
    binutil_build_configure = BUILD_ENV_ROOT + '/' + BINUTIL_SRC_DIR.format(BINUTIL_VERSION) +'/' + 'configure'
    gcc_build_configure = BUILD_ENV_ROOT + '/' + GCC_SRC_DIR.format(gcc_version) +'/' + 'configure'
    build_env_path = BUILD_ENV_ROOT + '/' + 'env-i386-gcc-' + gcc_version
    if os.path.isdir(build_env_path):
        print(build_env_path + ' exists.')
        continue
    call([binutil_build_configure,
        '--prefix=' + build_env_path,
        '--target=' + 'i386-linux-gnu',
        '--disable-multilib',
    ])
    call(['make', '-j16'])
    call(['make', 'install'])
    swicth_to_toolchain_build_dir(initial_working_dir)
    call([gcc_build_configure,
        '--prefix=' + build_env_path,
        '--target=' + 'i386-linux-gnu',
        '--disable-multilib',
        '--enable-languages=c',
        '--without-headers',
    ])
    call(['make', '-j16', 'all-gcc'])
    call(['make', 'install-gcc'])

os.chdir(initial_working_dir)
call(['rm', '-rf', TOOLCHAIN_BUILD_DIR])
