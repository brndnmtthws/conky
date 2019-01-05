#
# Conky, a system monitor, based on torsmo
#
# Please see COPYING for details
#
# Copyright (c) 2005-2019 Brenden Matthews, et. al. (see AUTHORS) All rights
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
# ${db2x_xsltproc_cmd} -s man ${srcdir}/docs.xml -o docs.mxml ${db2x_manxml_cmd}
# docs.mxml ${xsltproc_cmd} ${srcdir}/docbook-xml/docbook.xsl ${srcdir}/docs.xml
# > docs.html man -P 'less -is' ./conky.1 > README sed -i
# "s/[[:cntrl:]]\[[0-9]*m//g" README sed -i "s/\xE2//g" README sed -i
# "s/\x80//g" README sed -i "s/\x90/-/g" README mv README ${top_srcdir}
# ${xsltproc_cmd} ${srcdir}/variables.xsl ${srcdir}/variables.xml >
# variables.html ${xsltproc_cmd} ${srcdir}/config_settings.xsl
# ${srcdir}/config_settings.xml > config_settings.html ${xsltproc_cmd}
# ${srcdir}/lua.xsl ${srcdir}/lua.xml > lua.html

# else conky.1:

# endif

# man_MANS = conky.1

if(MAINTAINER_MODE)

  function(wrap_xsltproc)
    if(NOT ARGV)
      message(SEND_ERROR "Error: wrap_xsltproc called without any files")
      return()
    endif(NOT ARGV)

    foreach(FIL ${ARGV})
      add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html
                         COMMAND ${APP_XSLTPROC}
                                 ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xsl
                                 ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xml >
                                 ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html
                         DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xsl
                                 ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xml
                         COMMENT "Running xsltproc on ${FIL}")
      add_custom_target(${FIL}.html ALL
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html)
    endforeach(FIL)

  endfunction(wrap_xsltproc)

  function(wrap_man)
    if(NOT ARGV)
      message(SEND_ERROR "Error: wrap_man called without any files")
      return()
    endif(NOT ARGV)

    foreach(FIL ${ARGV})
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html
               ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.1
               ${CMAKE_SOURCE_DIR}/README
        COMMAND ${APP_XSLTPROC}
                ${CMAKE_SOURCE_DIR}/doc/docbook-xml/html/docbook.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xml >
                ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html
        COMMAND ${APP_DB2X_XSLTPROC} -s man
                ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xml -o ${FIL}.mxml
        COMMAND ${APP_DB2X_MANXML}
                --encoding=utf-8 ${FIL}.mxml
                --output-dir ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${APP_XSLTPROC}
                ${CMAKE_SOURCE_DIR}/doc/docbook-xml/html/docbook.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xml >
                ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html
        COMMAND ${APP_MAN} -P '${APP_LESS} -is'
                ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.1 >
                ${CMAKE_SOURCE_DIR}/README
        COMMAND ${APP_SED} -i'' -e 's/[[:cntrl:]]\\[[0-9]*m//g'
                ${CMAKE_SOURCE_DIR}/README
        COMMAND ${APP_SED} -i'' -e 's/\\xE2//g' ${CMAKE_SOURCE_DIR}/README
        COMMAND ${APP_SED} -i'' -e 's/\\x80//g' ${CMAKE_SOURCE_DIR}/README
        COMMAND ${APP_SED} -i'' -e 's/\\x90/-/g' ${CMAKE_SOURCE_DIR}/README
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.xml
        COMMENT "Proccessing man page for ${FIL}")
      add_custom_target(${FIL} ALL
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FIL}.html)
    endforeach(FIL)

  endfunction(wrap_man)

endif(MAINTAINER_MODE)
