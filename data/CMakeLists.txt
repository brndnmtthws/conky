# vim: ts=4 sw=4 noet ai cindent syntax=cmake
#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2010 Brenden Matthews, et. al. (see AUTHORS)
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

if(BUILD_CONFIG_OUTPUT)
	# generate defconfig
	add_custom_command(OUTPUT defconfig.h COMMAND
		${CMAKE_SOURCE_DIR}/text2c.sh ${CMAKE_CURRENT_SOURCE_DIR}/conky.conf
		defconfig.h defconfig DEPENDS conky.conf
	)
	add_custom_target(defconfig ALL DEPENDS defconfig.h)
	set_source_files_properties(defconfig.h PROPERTIES GENERATED TRUE)
endif(BUILD_CONFIG_OUTPUT)

