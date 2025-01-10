#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2024 Brenden Matthews, et. al. (see AUTHORS) All rights
# reserved.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details. You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

set(SUPPORTED_OS FALSE)

macro(CHECK_OS OS VARIABLE)
  if(CMAKE_SYSTEM_NAME MATCHES "${OS}")
    set(${VARIABLE} 1 CACHE INTERNAL "Operating system is ${OS}")
    set(SUPPORTED_OS TRUE)
  else()
    set(${VARIABLE} 0 CACHE INTERNAL "Operating system is ${OS}")
  endif()
endmacro()

check_os("Linux"     "OS_LINUX")
check_os("FreeBSD"   "OS_FREEBSD")
check_os("DragonFly" "OS_DRAGONFLY")
check_os("OpenBSD"   "OS_OPENBSD")
check_os("SunOS"     "OS_SOLARIS")
check_os("NetBSD"    "OS_NETBSD")
check_os("Haiku"     "OS_HAIKU")
check_os("Darwin"    "OS_DARWIN")

if(NOT SUPPORTED_OS)
  message(
    FATAL_ERROR
    "Your platform, '${CMAKE_SYSTEM_NAME}', is not currently supported. Patches are welcome."
  )
endif()

# Detect CI
if(DEFINED ENV{CI})
  # For GitHub actions CI=true is set
  set(ENV_IS_CI true CACHE INTERNAL "CI build environment")
  mark_as_advanced(ENV_IS_CI)
endif()
