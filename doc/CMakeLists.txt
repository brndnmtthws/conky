#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2021 Brenden Matthews, et. al. (see AUTHORS) All rights
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

if(BUILD_DOCS)
    add_custom_target(man.md
        ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docgen.py man.md.j2 > ${CMAKE_CURRENT_BINARY_DIR}/man.md
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/man.md
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES config_settings.yaml variables.yaml lua.yaml man.md.j2
    )
    add_custom_target(conky.1
        ALL
        COMMAND ${APP_PANDOC} -f markdown -t man ${CMAKE_CURRENT_BINARY_DIR}/man.md > ${CMAKE_CURRENT_BINARY_DIR}/conky.1
        SOURCES config_settings.yaml variables.yaml lua.yaml man.md.j2
    )
    add_dependencies(conky.1 man.md)
 
    set(MAN_PATH "share/man/man1")
    set(MAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/conky.1)

    install(FILES ${DOC_FILES} DESTINATION ${DOC_PATH})

    if(BUILD_DOCS)
        install(FILES ${MAN_FILES} DESTINATION ${MAN_PATH})
    endif(BUILD_DOCS)

endif(BUILD_DOCS)
