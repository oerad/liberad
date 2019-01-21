cmake_minimum_required (VERSION 3.0)

#info about liberad
set(LIBERAD_MAJOR 1)
set(LIBERAD_MINOR 0)
set(LIBERAD_PATCH 0)
set(LIBERAD_VERSION ${LIBERAD_MAJOR}.${LIBERAD_MINOR}.${LIBERAD_PATCH})
set(LIBERAD_DESCRIPTION "Liberad is a free open-source library for interfacing with hardware produced by Oerad Tech Ltd.")
set(LIBERAD_PACKAGE_NAME "liberad-${LIBERAD_VERSION}")

project (liberad
        LANGUAGES CXX
        VERSION ${LIBERAD_VERSION}
        DESCRIPTION ${LIBERAD_DESCRIPTION})


include(GNUInstallDirs)

add_library(liberad SHARED
            src/liberad.cpp)

target_link_libraries(liberad usb-1.0)

set_target_properties(liberad PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/liberad.h
    PRIVATE_HEADER include/EradLogger.h)

configure_file(liberad.pc.in liberad.pc @ONLY)

target_include_directories(liberad PRIVATE .)

install(TARGETS liberad
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/liberad
        PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/liberad)

install (FILES ${CMAKE_BINARY_DIR}/liberad.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
