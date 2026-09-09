# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.16)
project(xpenguins VERSION 2.2 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XPM REQUIRED xpm)

# Optional: Xfixes for empty input shape (click-through overlay)
pkg_check_modules(XFIXES xfixes)
# Shape extension is via Xext typically
find_library(XEXT_LIBRARY Xext REQUIRED)

set(XPENGUINS_SOURCES
  src/main.c
  src/xpenguins_config.c
  src/xpenguins_core.c
  src/xpenguins_theme.c
  src/toon_associate.c
  src/toon_draw.c
  src/toon_globals.c
  src/toon_query.c
  src/toon_set.c
  src/toon_core.c
  src/toon_end.c
  src/toon_init.c
  src/toon_root.c
  src/toon_signal.c
)

add_executable(xpenguins ${XPENGUINS_SOURCES})

target_include_directories(xpenguins PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${X11_INCLUDE_DIR}
  ${XPM_INCLUDE_DIRS}
)

target_compile_definitions(xpenguins PRIVATE
  HAVE_CONFIG_H
  PKGDATADIR="${CMAKE_INSTALL_FULL_DATADIR}/xpenguins"
)

if(XFIXES_FOUND)
  target_compile_definitions(xpenguins PRIVATE HAVE_XFIXES)
  target_include_directories(xpenguins PRIVATE ${XFIXES_INCLUDE_DIRS})
  target_link_libraries(xpenguins PRIVATE ${XFIXES_LIBRARIES})
endif()

target_link_libraries(xpenguins PRIVATE
  ${X11_LIBRARIES}
  ${XPM_LIBRARIES}
  ${XEXT_LIBRARY}
  m
)

# Generate a minimal config.h matching what the sources expect
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  @ONLY
)

install(TARGETS xpenguins RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES xpenguins.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Themes
set(THEME_DIRS Penguins Big_Penguins Classic_Penguins Turtles Bill)
foreach(theme ${THEME_DIRS})
  file(GLOB theme_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "themes/${theme}/*")
  # Exclude Makefile.am from install
  list(FILTER theme_files EXCLUDE REGEX "Makefile")
  install(FILES ${theme_files}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/xpenguins/themes/${theme})
endforeach()
