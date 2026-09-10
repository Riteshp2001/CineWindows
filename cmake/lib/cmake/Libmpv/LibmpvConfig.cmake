# Copyright (c) 2026 Ritesh Pandit
# Last modified: 2026-09-10
# Modified by: Ritesh Pandit

# Minimal LibmpvConfig.cmake – locates libmpv via pkg-config or find_library
# so MpvQt's find_package(Libmpv) succeeds without a distro-shipped config.

include(CMakeFindDependencyMacro)
find_dependency(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(PC_LIBMPV QUIET libmpv)
endif()

find_library(_libmpv_location NAMES mpv)
find_path(_libmpv_incdir mpv/client.h PATH_SUFFIXES include)

if(NOT _libmpv_location)
    set(Libmpv_FOUND FALSE PARENT_SCOPE)
    return()
endif()

set(Libmpv_FOUND TRUE)
set(Libmpv_VERSION "0.0.0")
set(Libmpv_INCLUDE_DIRS ${_libmpv_incdir})
set(Libmpv_LIBRARIES ${_libmpv_location})

if(NOT TARGET Libmpv::Libmpv)
    add_library(Libmpv::Libmpv UNKNOWN IMPORTED)
    set_target_properties(Libmpv::Libmpv PROPERTIES
        IMPORTED_LOCATION "${_libmpv_location}"
        INTERFACE_INCLUDE_DIRECTORIES "${_libmpv_incdir}"
    )
endif()
