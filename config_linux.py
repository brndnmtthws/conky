
## This is the linux configuration file
# use 'scons -h' to see the list of command line options available

# Compiler flags (based on Debian's installation of lua)
#LINKFLAGS = ['-g']
CCFLAGS = ['-I/usr/include/lua50', '-O2', '-ansi', '-Wall']
#CCFLAGS = ['-I/usr/include/lua50', '-g']

# this is the default directory for installation. Files will be installed on
# <prefix>/bin, <prefix>/lib and <prefix>/include when you run 'scons install'
#
# You can also specify this directory on the command line with the 'prefix'
# option
#
# You can see more 'generic' options for POSIX systems on config_posix.py

prefix = '/usr/local'

# libraries (based on Debian's installation of lua)
LIBS = ['lua50', 'lualib50', 'dl', 'm']

