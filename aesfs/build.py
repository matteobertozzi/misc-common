#!/usr/bin/env python
#
#   Copyright 2011 Matteo Bertozzi
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from commands import getstatusoutput as execCommand
from multiprocessing import cpu_count
from contextlib import contextmanager
from collections import deque
from threading import Thread

import shutil
import string
import time
import sys
import os

SYSTEM_CPU_COUNT = cpu_count()
NO_OUTPUT = False

def msg_write(*args):
    if not NO_OUTPUT:
        data = ' '.join([str(x) for x in args]) if len(args) > 0 else ''
        sys.stdout.write('%s\n' % data)

@contextmanager
def bench(format):
    st = time.time()
    try:
        yield
    finally:
        et = time.time()
        msg_write(format, (et - st))

def writeFile(filename, content):
    fd = open(filename, 'w')
    try:
        fd.write(content)
    finally:
        fd.close()

def _removeDirectory(path):
    if os.path.exists(path):
        for root, dirs, files in os.walk(path, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        os.removedirs(path)

def removeDirectoryContents(path):
    for root, dirs, files in os.walk(path, topdown=False):
        for name in dirs:
            _removeDirectory(os.path.join(root, name))

def _findCompiler():
    paths = os.getenv('PATH', '/usr/bin').split(':')
    if Build.platformIsMac():
        compilers = ['clang', 'gcc']
    else:
        compilers = ['gcc', 'clang']

    for compiler in compilers:
        for path in paths:
            compiler_path = os.path.join(path, compiler)
            if os.path.exists(compiler_path):
                return compiler_path

    raise Exception('Compiler Not Found!')

def ldLibraryPathUpdate(ldlibs):
    env_name = 'LD_LIBRARY_PATH'
    env = os.environ.get(env_name, '')
    env_libs = ':'.join([lib for lib in ldlibs if lib not in env])
    if env_libs:
        env = '%s:%s' % (env, env_libs) if env else env_libs
        os.environ[env_name] = env

class ThreadPool(object):
    class Worker(Thread):
        def __init__(self, pool):
            Thread.__init__(self)
            self.pool = pool
            self.start()

        def run(self):
            pool = self.pool
            while pool.running:
                try:
                    job_func = pool.jobs.popleft()
                except IndexError:
                    time.sleep(0)
                    continue

                try:
                    result = job_func()
                    pool.job_results.append(result)
                except:
                    pool.job_failures += 1

    def __init__(self):
        self.running = True
        self.jobs = deque()
        self.job_results = []
        self.job_failures = 0
        self._workers = [self.Worker(self) for _ in xrange(SYSTEM_CPU_COUNT)]

    def push(self, job, *args, **kwargs):
        self.jobs.append(lambda: job(*args, **kwargs))

    def join(self, raise_failure=True):
        while len(self.jobs) > 0:
            if raise_failure and self.job_failures > 0:
                break

        self.running = False
        for worker in self._workers:
            worker.join()

        if raise_failure and self.job_failures > 0:
            raise Exception('%d Job Failures' % self.job_failures)

        return self.job_results

    def hasFailures(self):
        return self.job_failures > 0

class BuildOptions(object):
    def __init__(self):
        self.cc = _findCompiler()
        self.ldlibs = set()
        self.cflags = set()
        self.defines = set()
        self.includes = set()
        self.pedantic = False

    def setCompiler(self, cc):
        self.cc = cc

    def addCFlags(self, cflags):
        self.cflags |= set(cflags)

    def addDefines(self, defines):
        self.defines |= set(defines)

    def addIncludePaths(self, includes):
        self.includes |= set(includes)

    def addLdLibs(self, ldlibs):
        self.ldlibs |= set(ldlibs)

    def setPedantic(self, pedantic):
        self.pedantic = pedantic

    def clone(self):
        opts = BuildOptions()
        opts.setCompiler(self.cc)
        opts.addCFlags(self.cflags)
        opts.addDefines(self.defines)
        opts.addIncludePaths(self.includes)
        opts.addLdLibs(self.ldlibs)
        opts.setPedantic(self.pedantic)
        return opts

class Build(object):
    DEFAULT_BUILD_DIR = 'build'

    def __init__(self, name, build_dir=None, options=[]):
        def _setDefaultFunc(value, default_f):
            return value if value else default_f()

        def _setDefaultValue(value, default_v):
            return value if value else default_v

        self.name = name
        self._options = _setDefaultFunc(options, BuildOptions)
        self._makeBuildDirs(_setDefaultValue(build_dir, self.DEFAULT_BUILD_DIR))

    def build(self, *args, **kwargs):
        self.cleanup()
        os.makedirs(self._dir_obj)
        os.makedirs(self._dir_lib)
        os.makedirs(self._dir_inc)
        return self._build(*args, **kwargs)

    def _build(self):
        raise NotImplementedError

    def cleanup(self, full=True):
        if full:
            _removeDirectory(self._dir_out)
        else:
            _removeDirectory(self._dir_obj)

    def runTool(self, tool, verbose=True):
        ldLibraryPathUpdate([self._dir_lib])
        exit_code, output = execCommand(tool)

        tool_output = []
        if verbose:
            tool_output.append(output)

        if exit_code != 0:
            tool_output.append(' [FAIL] %s exit code %d' % (tool, exit_code))
        else:
            tool_output.append(' [ OK ] %s' % tool)

        msg_write('\n'.join(tool_output))

    def runTools(self, name, tools, verbose=True):
        msg_write('Run Tools:', name)
        msg_write('-' * 60)

        pool = ThreadPool()
        for tool in tools:
            pool.push(self.runTool, tool, verbose)
        pool.join()

        msg_write()

    def _makeBuildDirs(self, build_dir):
        self._file_buildnr = os.path.join(build_dir, '.buildnr-%s' % self.name)
        self._dir_out = os.path.join(build_dir, self.name)
        self._dir_obj = os.path.join(self._dir_out, 'objs')
        self._dir_lib = os.path.join(self._dir_out, 'libs')
        self._dir_inc = os.path.join(self._dir_out, 'include')

    def compileFile(self, filename, dump_error=True):
        obj_name, obj_path = self._objFilePath(filename)

        cmd = '%s -c %s %s %s %s %s -o %s' %                \
               (self._options.cc,                           \
                string.join(self._options.cflags, ' '),     \
                string.join(self._options.defines, ' '),    \
                string.join(self._options.includes, ' '),   \
                '-I%s' % self._dir_inc,                     \
                filename,                                   \
                obj_path)

        msg_write(' [CC]', filename)
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            if dump_error:
                msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
            raise RuntimeError("Compilation Failure!")

        if self._options.pedantic and len(output) > 0:
            msg_write(output)

    def compileDirectories(self, dirs_src):
        pool = ThreadPool()
        compileFunc = lambda f: pool.push(self.compileFile, f)
        for src in dirs_src:
            self._cFilesWalk(src, compileFunc)
        results = pool.join()
        return len(results)

    def linkFile(self, filename, dump_error=True):
        _, obj_path = self._objFilePath(filename)
        app_name, app_path = self._appFilePathFromObj(obj_path)

        cmd = '%s -o %s %s %s' %                            \
                (self._options.cc, app_path, obj_path,      \
                 string.join(self._options.ldlibs, ' '))
        msg_write(' [LD]', app_name)
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            if dump_error:
                msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
            raise RuntimeError("Linking Failure!")

        return app_path

    def _objFilePath(self, filename):
        if filename.endswith('.o'):
            return os.path.basename(filename), filename
        objname = os.path.normpath(filename).replace('/', '_')
        objname = objname[:objname.rindex('.')] + '.o'
        objpath = os.path.join(self._dir_obj, objname)
        return objname, objpath

    def _appFilePath(self, filename):
        obj_path = self._objFilePath(filename)
        return self._appFilePathFromObj(obj_path)

    def _appFilePathFromObj(self, obj_path):
        app_name = obj_path[obj_path.rfind('_') + 1:-2]
        app_path = os.path.join(self._dir_out, app_name)
        return app_name, app_path

    def _cFilesWalk(self, dir_src, func):
        for root, dirs, files in os.walk(dir_src, topdown=False):
            for name in files:
                if name.endswith('.c') or name.endswith('.cc'):
                    func(os.path.join(root, name))

    def buildNumber(self, major, minor, inc=1):
        mx = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'

        try:
            build = int(file(self._file_buildnr, 'r').read()) + inc
        except:
            build = 0

        if inc > 0:
            file(self._file_buildnr, 'w').write('%d' % build)

        return '%d%s%04X' % (major, mx[minor], build)

    @staticmethod
    def platformIsMac():
        return os.uname()[0] == 'Darwin'

    @staticmethod
    def platformIsLinux():
        return os.uname()[0] == 'Linux'

class BuildApp(Build):
    def __init__(self, name, src_dirs, **kwargs):
        super(BuildApp, self).__init__(name, **kwargs)
        self.src_dirs = src_dirs

    def _build(self):
        msg_write('Building', self.name)
        msg_write('-' * 60)

        if not self.compileDirectories(self.src_dirs):
            return

        obj_list = os.listdir(self._dir_obj)
        obj_list = [os.path.join(self._dir_obj, f) for f in obj_list]
        app_path = os.path.join(self._dir_out, self.name)

        cmd = '%s -o %s %s %s' %                            \
                (self._options.cc, app_path,                \
                 string.join(obj_list, ' '),                \
                 string.join(self._options.ldlibs, ' '))

        msg_write(' [LD]', self.name)
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
            sys.exit(1)
        msg_write()

class BuildMiniTools(Build):
    def __init__(self, name, src_dirs, **kwargs):
        super(BuildMiniTools, self).__init__(name, **kwargs)
        self.src_dirs = src_dirs

    def _build(self):
        msg_write('Building', self.name)
        msg_write('-' * 60)

        if not self.compileDirectories(self.src_dirs):
            return []

        pool = ThreadPool()
        for obj_name in os.listdir(self._dir_obj):
            pool.push(self.linkFile, os.path.join(self._dir_obj, obj_name))
        tools = pool.join()

        msg_write()
        return sorted(tools)

class BuildConfig(Build):
    def __init__(self, name, src_dirs, **kwargs):
        super(BuildConfig, self).__init__(name, **kwargs)
        self.src_dirs = src_dirs

    def _build(self, config_file, config_head, debug=False, dump_error=False):
        msg_write('Running Build.Config')
        msg_write('-' * 60)

        pool = ThreadPool()
        test_func = lambda f: pool.push(self._testApp, f, dump_error)
        for src_dir in self.src_dirs:
            self._cFilesWalk(src_dir, test_func)
        config = pool.join(raise_failure=False)

        self._writeConfigFile(config_file, config_head, config, debug)

    def _testApp(self, filename, dump_error):
        try:
            self.compileFile(filename, dump_error=dump_error)
            self.linkFile(filename, dump_error=dump_error)
        except:
            msg_write(' [!!]', filename)
            raise Exception('Config Test %s failed' % filename)

        obj_name, obj_path = self._objFilePath(filename)
        app_name, app_path = self._appFilePathFromObj(obj_path)

        ldLibraryPathUpdate([self._dir_lib])
        exit_code, output = execCommand(app_path)
        if exit_code != 0:
            msg_write(' [!!]', filename)
            raise Exception('Config Test %s failed' % app_name)

        return app_name

    def _writeConfigFile(self, config_file, config_head, config, debug):
        msg_write(' [WR] Write config', config_file)
        fd = open(config_file, 'w')
        fd.write('#ifndef _%s_BUILD_CONFIG_H_\n' % config_head)
        fd.write('#define _%s_BUILD_CONFIG_H_\n' % config_head)
        fd.write('\n')

        fd.write('/* C++ needs to know that types and declarations are C, not C++. */\n')
        fd.write('#ifdef __cplusplus\n')
        fd.write('    #define __%s_BEGIN_DECLS__         extern "C" {\n' % config_head)
        fd.write('    #define __%s_END_DECLS__           }\n' % config_head)
        fd.write('#else\n')
        fd.write('    #define __%s_BEGIN_DECLS__\n' % config_head)
        fd.write('    #define __%s_END_DECLS__\n' % config_head)
        fd.write('#endif\n')
        fd.write('\n')

        if debug:
            fd.write('/* Debugging Mode on! Print as much as you can! */\n')
            fd.write('#define __Z_DEBUG__      1\n')
            fd.write('\n')

        if len(config) > 0:
            fd.write("/* You've support for this things... */\n")

        for define in config:
            fd.write('#define %s_%s\n' % (config_head, define.upper().replace('-', '_')))

        fd.write('\n')
        fd.write('#endif /* !_%s_BUILD_CONFIG_H_ */\n' % config_head)
        fd.close()

        msg_write()

class BuildLibrary(Build):
    SKIP_HEADER_ENDS = ('_p.h', 'private.h')

    def __init__(self, name, version, src_dirs, copy_dirs=None, **kwargs):
        super(BuildLibrary, self).__init__(name, **kwargs)
        self.version = version
        self.src_dirs = src_dirs
        self.copy_dirs = copy_dirs or []

    def moveBuild(self, dst):
        _removeDirectory(dst)
        os.makedirs(dst)
        os.rename(self._dir_inc, os.path.join(dst, 'include'))
        os.rename(self._dir_lib, os.path.join(dst, 'libs'))

    def _build(self):
        v_maj, v_min, v_rev = (int(x) for x in self.version.split('.'))
        build_nr = self.buildNumber(v_maj, v_min)

        msg_write('Copy %s %s (%s) Library Headers' % (self.name, self.version, build_nr))
        msg_write('-' * 60)
        self.copyHeaders()

        msg_write('Building %s %s (%s) Library' % (self.name, self.version, build_nr))
        msg_write('-' * 60)

        if not self.compileDirectories(self.src_dirs):
            return

        obj_list = os.listdir(self._dir_obj)
        obj_list = [os.path.join(self._dir_obj, f) for f in obj_list]

        if not os.path.exists(self._dir_lib):
            os.makedirs(self._dir_lib)

        libversion_maj = self.version[:self.version.index('.')]
        lib_ext = 'dylib' if self.platformIsMac() else 'so'
        lib_name = 'lib%s.%s' % (self.name, lib_ext)
        lib_name_maj = 'lib%s.%s.%s' % (self.name, lib_ext, libversion_maj)
        lib_name_full = 'lib%s.%s.%s' % (self.name, lib_ext, self.version)
        lib_path = os.path.join(self._dir_lib, lib_name_full)

        if self.platformIsMac():
            cmd = '%s -dynamiclib -current_version %s -o %s -dylib %s %s' % \
                    (self._options.cc, self.version, lib_path,              \
                     string.join(obj_list, ' '),                            \
                     string.join(self._options.ldlibs, ' '))
        elif self.platformIsLinux():
            cmd = '%s -shared -Wl,-soname,%s -o %s %s %s' %                 \
                    (self._options.cc, lib_name_maj, lib_path,              \
                     string.join(obj_list, ' '),                            \
                     string.join(self._options.ldlibs, ' '))
        else:
            raise RuntimeError("Unsupported Platform %s" % ' '.join(os.uname()))

        msg_write()
        msg_write(' [LD]', lib_name_full)
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
            sys.exit(1)

        cwd = os.getcwd()
        os.chdir(self._dir_lib)
        for name in (lib_name, lib_name_maj):
            msg_write(' [LN]', name)
            execCommand('ln -s %s %s' % (lib_name_full, name))
        os.chdir(cwd)

        msg_write()

    def copyHeaders(self):
        self.copyHeadersFromTo(None, self.src_dirs)
        for hname, hdirs in self.copy_dirs:
            self.copyHeadersFromTo(hname, hdirs)
        msg_write()

    def copyHeadersFromTo(self, name, src_dirs):
        dir_dst = os.path.join(self._dir_inc, self.name)
        if name is not None:
            dir_dst = os.path.join(dir_dst, name)
        _removeDirectory(dir_dst)
        os.makedirs(dir_dst)

        for dir_src in src_dirs:
            for root, dirs, files in os.walk(dir_src, topdown=False):
                for name in files:
                    if not name.endswith('.h'):
                        continue

                    for s in self.SKIP_HEADER_ENDS:
                        if name.endswith(s):
                            break
                    else:
                        src_path = os.path.join(root, name)
                        dst_path = os.path.join(dir_dst, name)
                        shutil.copyfile(src_path, dst_path)
                        msg_write(' [CP]', dst_path)

def _parseCmdline():
    try:
        from argparse import ArgumentParser
    except ImportError:
        from optparse import OptionParser
        class ArgumentParser(OptionParser):
            def add_argument(self, *args, **kwargs):
                return self.add_option(*args, **kwargs)

            def parse_args(self):
                options, args = OptionParser.parse_args(self)
                return options

    parser = ArgumentParser()
    parser.add_argument('-c', '--compiler', dest='compiler', action='store',
                        help='Compiler to use')

    parser.add_argument('-r', '--release', dest='release', action='store_true', default=False,
                        help="Use release flags during compilation")
    parser.add_argument('-p', '--pedantic', dest='pedantic', action='store_true', default=False,
                        help="Issue all the warnings demanded by strict ISO C")
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true', default=False,
                        help="Show traceback infomation if something fails")
    parser.add_argument('--no-output', dest='no_output', action='store_true', default=False,
                        help='Do not print messages')

    return parser.parse_args()

if __name__ == '__main__':
    options = _parseCmdline()

    NO_OUTPUT = options.no_output

    DEFAULT_CFLAGS = ['-Wall', '-Wmissing-field-initializers']
    DEFAULT_RELEASE_CFLAGS = ['-O2']
    DEFAULT_DEBUG_CFLAGS = ['-g']
    DEFAULT_DEFINES = ['-D__USE_FILE_OFFSET64', '-D_FILE_OFFSET_BITS=64', '-D_XOPEN_SOURCE=700']
    DEFAULT_LDLIBS = []

    if Build.platformIsMac():
        DEFAULT_DEFINES.extend(['-DCRYPTO_COMMON_CRYPTO'])
    else:
        DEFAULT_DEFINES.extend(['-DHAVE_SETXATTR', '-DCRYPTO_OPENSSL'])
        DEFAULT_LDLIBS.extend(['-lcrypto'])

    # Default Build Options
    default_opts = BuildOptions()
    default_opts.addDefines(DEFAULT_DEFINES)
    default_opts.addLdLibs(DEFAULT_LDLIBS)
    default_opts.addCFlags(DEFAULT_CFLAGS)
    default_opts.setPedantic(options.pedantic)

    if options.compiler is not None:
        default_opts.setCompiler(options.compiler)

    if options.release:
        default_opts.addCFlags(DEFAULT_RELEASE_CFLAGS)
    else:
        default_opts.addCFlags(DEFAULT_DEBUG_CFLAGS)

    if options.pedantic:
        default_opts.addCFlags(['-pedantic', '-Wignored-qualifiers',
                                '-Wsign-compare', '-Wtype-limits',
                                '-Wuninitialized', '-Winline', '-Wpacked'])
    else:
        default_opts.addCFlags(['-Werror'])

    # Compute ld libs/includes path
    _ldlib = lambda name: '-L%s/%s/libs -l%s' % (Build.DEFAULT_BUILD_DIR, name, name)
    _inclib = lambda name: '-I%s/%s/include' % (Build.DEFAULT_BUILD_DIR, name)

    with bench('[T] Build Time'):
        default_opts.addIncludePaths(['-I./src'])

        build_opts = default_opts.clone()
        if Build.platformIsMac():
            build_opts.addIncludePaths(['-I/usr/local/include/osxfuse/fuse'])
            build_opts.addLdLibs(['-lpthread', '-L/usr/local/lib/ -losxfuse', '-ldl'])
        else:
            build_opts.addIncludePaths(['-I/usr/include/fuse'])
            build_opts.addLdLibs(['-lpthread', '-lfuse', '-lrt', '-ldl'])
        build = BuildApp('aesfs', ['tool-fuse', 'src'], options=build_opts)
        build.build()

        build = BuildApp('aespack', ['tool-pack', 'src'], options=build_opts)
        build.build()

