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

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nano)
add_custom_target(conky.nanorc
    ALL
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/doc/render.py nano/conky.nanorc.j2 > ${CMAKE_CURRENT_BINARY_DIR}/nano/conky.nanorc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES
    ${CMAKE_SOURCE_DIR}/doc/variables.yaml
    ${CMAKE_SOURCE_DIR}/doc/config_settings.yaml
    ${CMAKE_SOURCE_DIR}/doc/lua.yaml
    ${CMAKE_CURRENT_SOURCE_DIR}/nano/conky.nanorc.j2
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/nano/conky.nanorc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/nano)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vim/syntax)
add_custom_target(conkyrc.vim
    ALL
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/doc/render.py vim/syntax/conkyrc.vim.j2 > ${CMAKE_CURRENT_BINARY_DIR}/vim/syntax/conkyrc.vim
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES
    ${CMAKE_SOURCE_DIR}/doc/variables.yaml
    ${CMAKE_SOURCE_DIR}/doc/config_settings.yaml
    ${CMAKE_SOURCE_DIR}/doc/lua.yaml
    ${CMAKE_CURRENT_SOURCE_DIR}/vim/syntax/conkyrc.vim.j2
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vim/syntax/conkyrc.vim DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/vim/syntax)
