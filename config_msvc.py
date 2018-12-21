
## This is the MSVC configuration file
# use 'scons -h' to see the list of command line options available

# flags for the compiler
CCFLAGS = ['/nologo']

# this is the default directory for installation. Files will be installed on
# <prefix>/bin, <prefix>/lib and <prefix>/include when you run 'scons install'
#
# You can also specify this directory on the command line with the 'prefix'
# option
#
# If you leave it as 'None', we'll try to auto-detect it (as 'PLATFORMSDKDIR'
# detected by SCons).

prefix = None # (it's a string)

# the libraries
LIBS = ['lua', 'lualib']

# linkflags
LINKFLAGS = ['/nologo']

## We need to specifiy the environment for the PATH and LIB and all those
# parameters cl tales from it
import os
ENV = os.environ
