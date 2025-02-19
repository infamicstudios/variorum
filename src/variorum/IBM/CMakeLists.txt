# Copyright 2019-2023 Lawrence Livermore National Security, LLC and other
# Variorum Project Developers. See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

set(variorum_ibm_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/Power9.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ibm_power_features.h
  CACHE INTERNAL "")

set(variorum_ibm_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Power9.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ibm_power_features.c
  CACHE INTERNAL "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${variorum_includes})

add_library(variorum_ibm OBJECT
            ${variorum_ibm_sources}
            ${variorum_ibm_headers})

### Shared libraries need PIC
set_property(TARGET ${variorum_ibm} PROPERTY POSITION_INDEPENDENT_CODE 1)
