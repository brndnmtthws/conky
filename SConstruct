import sys;
import os

tools = ['default']
if os.name == 'nt':
	tools = ['mingw']

env = Environment(tools = tools)

options_file = None
if sys.platform == 'linux2':
	options_file = "linux"

elif 'msvc' in env['TOOLS']:
	options_file = "msvc"
else:
	options_file = "posix"

opts = Options(["config_"+options_file+".py", "custom.py", "custom_"+options_file+".py"], ARGUMENTS)
opts.Add('CC', 'The C compiler.')
opts.Add('CXX', 'The C++ compiler (for the tests)')
opts.Add('CCFLAGS', 'Flags for the compiler.', ['-O2', '-Wall'])
opts.Add('LINK', 'The linker.')
opts.Add('LINKFLAGS', 'Linker flags.', [])
opts.Add('no_cygwin', 'Use -mno-cygwin to build using the mingw compiler on cygwin', 0)
opts.Add('LIBS', 'libraries', [])
opts.Add('LIBPATH', 'library path', [])

opts.Add('tolua_bin', 'the resulting binary', 'tolua++')
opts.Add('tolua_lib', 'the resulting library', 'tolua++')
opts.Add('TOLUAPP', 'the name of the tolua++ binary (to use with built_dev=1)', 'tolua++')

opts.Add('prefix', 'The installation prefix')
opts.Add('build_dev', 'Build for development (uses tolua to rebuild toluabind.c with the embeded scripts', 0)
opts.Add('build_failsafe', "Build using 'factory default' toluabind file (in case build_dev fails)", 0)
opts.Add('ENV', 'The environment variables')
opts.Add('shared', 'Build a shared object', False)
opts.Update(env)
Help(opts.GenerateHelpText(env))

def save_config(target, source, env):
	opts.Save('custom.py', env)

cust = env.Command('custom.py', [], save_config)
env.Alias('configure', [cust])

env['TOLUAPP_BOOTSTRAP'] = env['tolua_bin']+"_bootstrap"+env['PROGSUFFIX']

env['build_dev'] = int(env['build_dev'])

## detecting the install directory on win32
if 'msvc' in env['TOOLS'] and not (env.has_key('prefix') or env['prefix']):

	if env['MSVS'].has_key('PLATFORMSDKDIR'):
		env['prefix'] = env['MSVS']['PLATFORMSDKDIR']


SConscriptChdir(0)

############ helper builders
def pkg_scan_dep(self, target, source):

	import re

	## TODO: detectar si el archivo existe antes de abrirlo asi nomas
	pkg = open(source, "rt")

	for linea in pkg.xreadlines():
		dep = re.search("^[\t\w]*\$[cphl]file\s*\"([^\"]+)\"", linea)
		if dep:
			self.Depends(target, '#' + dep.groups()[0]);

			if dep.groups()[0][-4:] == '.pkg':
				# recursividad
				self.pkg_scan_dep(target, dep.groups()[0])


def make_tolua_code(self, target, source, pkgname = None, bootstrap = False, use_own = False, use_typeid=None):

	ptarget = Dir('.').path + '/' + target
	psource = Dir('.').path + '/' + source
	header = target[:-2] + '.h'
	pheader = Dir('.').path + '/' + header

	tolua = ""
	if bootstrap:
		if os.name == 'nt':
			tolua = 'bin\\'+self['TOLUAPP_BOOTSTRAP']
		else:
			tolua = 'bin/'+self['TOLUAPP_BOOTSTRAP']
		print("********* tolua is ", tolua)
	else:
		if use_own:
			if 'msvc' in self['TOOLS']:
				tolua = 'bin\\$tolua_bin'
			else:
				tolua = 'bin/$tolua_bin'
		else:
			tolua = "$TOLUAPP"

	if pkgname:
		pkgname = ' -n '+pkgname
	else:
		pkgname = ''

	if use_typeid:
		tolua = tolua+' -t'

	comando = tolua + ' -C -H ' + pheader + ' -o ' + ptarget + pkgname + ' ' + psource
	command = self.Command(target, source, comando)

	self.SideEffect(header, target)
	self.Depends(target, source)

	self.pkg_scan_dep(target, psource)

	if bootstrap:
		self.Depends(target, "#/bin/$TOLUAPP_BOOTSTRAP")
	if use_own:
		self.Depends(target, "#/bin/$tolua_bin")
	
	return command


env.__class__.LuaBinding = make_tolua_code;
env.__class__.pkg_scan_dep = pkg_scan_dep;

def print_install_error(target, source, env):

	msg = """Error: no install prefix was specified, or detected.

you can use the 'prefix' option on command line to specify one. Examples:

	scons prefix=/usr/local install

or on Windows:

	scons "prefix=c:\\program files\\visual basic" install

Files will be installed on <prefix>/bin, <prefix>/lib and <prefix>/include
"""
	import SCons.Errors
	raise SCons.Errors.UserError(msg)

########### end of helper builders

env['CPPPATH'] = '#/include'
env['LIBPATH'] =  ['#/lib'] + env['LIBPATH']

if env['no_cygwin']:

	env['CCFLAGS'] += ['-mno-cygwin']
	env['LINKFLAGS'] += ['-mno-cygwin']

import string

Export('env')

SConscript('src/lib/SCsub')
SConscript('src/bin/SCsub')
#SConscript('src/lib/SCsub')
SConscript('src/tests/SCsub')

env.Alias('all', [env.bin_target, env.lib_target])
env.Alias('test', env.test_targets)

Default('all')

if env['prefix']:
	env.Install(env['prefix']+'/bin', env.bin_target)
	env.Install(env['prefix']+'/lib', env.lib_target)
	env.Install(env['prefix']+'/include', '#include/tolua++.h')

	env.Alias('install', [env['prefix']+'/bin', env['prefix']+'/include', env['prefix']+'/lib'])
else:
	env.Command('install', [], print_install_error)
	env.Depends('install', 'all')

env.Command('deb', [], 'dpkg-buildpackage -I.svn -Icustom.py -Itoluabind_dev.c -Itoluabind_dev.h -Itoluabind_default.o -Icustom.lua -I.sconsign', ENV=os.environ)

