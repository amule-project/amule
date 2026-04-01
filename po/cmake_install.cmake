# Install script for directory: /home/eli/git/amule/po

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ar/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ar.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ast/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ast.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/bg/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/bg.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ca/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ca.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/cs/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/cs.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/da/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/da.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/de/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/de.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/el/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/el.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/en_GB/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/en_GB.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/es/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/es.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/et_EE/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/et_EE.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/eu/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/eu.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/fi/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/fi.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/fr/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/fr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/gl/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/gl.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/he/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/he.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/hr/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/hr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/hu/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/hu.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/it/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/it.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/it_CH/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/it_CH.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ja/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ja.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ko_KR/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ko_KR.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/lt/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/lt.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/nl/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/nl.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/nn/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/nn.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pl/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/pl.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pt_BR/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/pt_BR.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pt_PT/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/pt_PT.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ro/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ro.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ru/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/ru.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/sl/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/sl.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/sq/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/sq.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/sv/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/sv.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/tr/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/tr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/uk/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/uk.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/zh_CN/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/zh_CN.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/zh_TW/LC_MESSAGES" TYPE FILE RENAME "amule.mo" FILES "/home/eli/git/amule/po/zh_TW.gmo")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/eli/git/amule/po/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
