# ------------------------------------------------------------------------------
# CMakeLists.txt for libghttp Project Top Level
# ------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.6.0)

project(libghttp LANGUAGES C)

include(GNUInstallDirs)

# ------------------------------------------------------------------------------
# OS Specific
# ------------------------------------------------------------------------------

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(LINUX TRUE)
  message(STATUS "Support for system: " ${CMAKE_SYSTEM_NAME})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  message(STATUS "Support for system: " ${CMAKE_SYSTEM_NAME})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message(STATUS "Support for system: " ${CMAKE_SYSTEM_NAME})
else()
  message(DEPRECATION "No support for system: " ${CMAKE_SYSTEM_NAME})
endif()

# ------------------------------------------------------------------------------
# Get and Display infomation
# ------------------------------------------------------------------------------

message(STATUS "CMake version: " ${CMAKE_VERSION})
message(STATUS "CMake system name: " ${CMAKE_SYSTEM_NAME})
message(STATUS "C compiler: " ${CMAKE_C_COMPILER})
message(STATUS "CXX compiler: " ${CMAKE_CXX_COMPILER})


# ------------------------------------------------------------------------------
# Build type
# ------------------------------------------------------------------------------

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
  CACHE STRING "Choose the type of build: None Debug Release Coverage" FORCE)

if(CMAKE_BUILD_TYPE)
  message(STATUS "CMake build type: " ${CMAKE_BUILD_TYPE})
endif()

# Default C / CXX flags
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
endif()

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG  -Wall")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb -O0 -Wall -fstack-protector-all")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb -O0 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

# add stace info for debug
if(${CMAKE_C_COMPILER_ID} MATCHES "GNU")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -funwind-tables -finstrument-functions")
endif()

set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_COVERAGE ${CMAKE_C_FLAGS_COVERAGE})

# ------------------------------------------------------------------------------
# Version
# ------------------------------------------------------------------------------

set(VERSION_MAJOR 2)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(RELEASE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
message(STATUS "${PROJECT_NAME} release version: " ${RELEASE_VERSION})

set(BUILD_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
# get_commit_id(BUILD_VERSION)
if(BUILD_VERSION)
  message(STATUS "${PROJECT_NAME} build version: " ${BUILD_VERSION})
endif()

# ------------------------------------------------------------------------------
# target static and shared
# ------------------------------------------------------------------------------

option(BUILD_STATIC "Build static library." OFF)
option(BUILD_SHARED "Build shared library." ON)

if (NOT BUILD_STATIC AND NOT BUILD_SHARED)
  message(FATAL_ERROR "Need to choose static or shared build!")
endif()


# ------------------------------------------------------------------------------
# SSL/TLS feature
# ------------------------------------------------------------------------------

option(WITH_WOLFSSL "Build with WOLFSSL." OFF)
option(WITH_MBEDTLS "Build with MBEDTLS." OFF)
option(WITH_OPENSSL "Build with OPENSSL." ON)

# TODO: Add support for STATIC also.
if(WITH_WOLFSSL)
    find_package(WolfSSL REQUIRED)
    message("WolfSSL include dir: ${WOLFSSL_INCLUDE_DIR}")
    message("WolfSSL libraries: ${WOLFSSL_LIBRARIES}")
    include_directories(${WOLFSSL_INCLUDE_DIR})
    link_directories(${WOLFSSL_LIBRARIES})

    list(APPEND LIB_LIST ${WOLFSSL_LIBRARIES})
    set(depends_external external-wolfssl)
elseif(WITH_MBEDTLS)
    find_package(MbedTLS REQUIRED)
    message("MbedTLS include dir: ${MBEDTLS_INCLUDE_DIR}")
    message("MbedTLS libraries: ${MBEDTLS_LIBRARIES}")
    include_directories(${MBEDTLS_INCLUDE_DIR})
    link_directories(${MBEDTLS_LIBRARIES})

    list(APPEND LIB_LIST ${MBEDTLS_LIBRARIES})
    set(depends_external external-mbedtls)
elseif(WITH_OPENSSL)
    find_package(OpenSSL REQUIRED)
    message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_directories(${OPENSSL_LIBRARIES})

    list(APPEND LIB_LIST ${OPENSSL_LIBRARIES})
    set(depends_external external-openssl)

    set(USE_SSL ON)
endif()


set(CURRENT_TARGET "ghttp")

set(SRCS_${CURRENT_TARGET}
    src/ghttp.c
    src/http_base64.c
    src/http_date.c
    src/http_hdrs.c
    src/http_req.c
    src/http_resp.c
    src/http_trans.c
    src/http_uri.c
)

foreach(f IN LISTS SRCS_${CURRENT_TARGET})
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILE_NAME__=\"${b}\"")
endforeach()

include_directories(${PROJECT_SOURCE_DIR}/include)

# build static target
if(BUILD_STATIC)
    add_library(${CURRENT_TARGET}_STATIC STATIC ${SRCS_${CURRENT_TARGET}})
    set_target_properties(${CURRENT_TARGET}_STATIC PROPERTIES OUTPUT_NAME ${CURRENT_TARGET})
    target_compile_definitions(${CURRENT_TARGET}_STATIC PRIVATE "-D__PROJECT__=\"[${CURRENT_TARGET}]\"")
    if (WITH_WOLFSSL OR WITH_MBEDTLS OR WITH_OPENSSL)
        add_dependencies(${CURRENT_TARGET}_STATIC ${depends_external})
    endif()
    install(TARGETS ${CURRENT_TARGET}_STATIC
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )
endif(BUILD_STATIC)

# build shared target
if(BUILD_SHARED)
    add_library(${CURRENT_TARGET}_SHARED SHARED ${SRCS_${CURRENT_TARGET}})
    set_target_properties(${CURRENT_TARGET}_SHARED PROPERTIES OUTPUT_NAME ${CURRENT_TARGET})
    set_target_properties(${CURRENT_TARGET}_SHARED PROPERTIES VERSION ${RELEASE_VERSION} SOVERSION ${VERSION_MAJOR})
    target_compile_definitions(${CURRENT_TARGET}_SHARED PRIVATE "-D__PROJECT__=\"[${CURRENT_TARGET}]\"")
    if(WITH_TLS)
        add_dependencies(${CURRENT_TARGET}_SHARED ${depends_external})
    endif()
    install(TARGETS ${CURRENT_TARGET}_SHARED
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )
endif(BUILD_SHARED)


add_custom_target(lib)
if(BUILD_STATIC)
    add_dependencies(lib ${CURRENT_TARGET}_STATIC)
endif()
if(BUILD_SHARED)
    add_dependencies(lib ${CURRENT_TARGET}_SHARED)
endif()




# ------------------------------------------------------------------------------
# Configuration file
# ------------------------------------------------------------------------------

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY)


find_program(DOXYGEN doxygen)
if(DOXYGEN)
  add_custom_target(doc
    COMMAND doxygen ghttp.Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc)
endif()

# ------------------------------------------------------------------------------
# Subdirectory
# ------------------------------------------------------------------------------

# export head file
file(GLOB ghttp_headers
  include/ghttp.h
  include/ghttp_constants.h
)

install(FILES ${ghttp_headers}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ghttp
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# ------------------------------------------------------------------------------
# Test
# ------------------------------------------------------------------------------

option(ENABLE_TESTING "Build tests." OFF)

if(ENABLE_TESTING)
  find_program(VALGRIND valgrind)
  if(VALGRIND)
    # Ctest configuration variables must be set BEFORE include(Ctest)
    set(MEMORYCHECK_COMMAND ${VALGRIND})
    # Check process children and give detail for each leak
    set(MEMORYCHECK_COMMAND_OPTIONS
      "${MEMORYCHECK_COMMAND_OPTIONS} --leak-check=yes --show-reachable=no --undef-value-errors=no")
    set(MEMORYCHECK_COMMAND_OPTIONS
      "${MEMORYCHECK_COMMAND_OPTIONS} --trace-children=yes --track-fds=no --track-origins=no")
    # Set error code.
    set(MEMORYCHECK_COMMAND_OPTIONS
      "${MEMORYCHECK_COMMAND_OPTIONS} --error-exitcode=255")
    # Generate suppression hint.
    set(MEMORYCHECK_COMMAND_OPTIONS
      "${MEMORYCHECK_COMMAND_OPTIONS} --gen-suppressions=all")
    # Set suppression file
    set(MEMORYCHECK_COMMAND_OPTIONS
      "${MEMORYCHECK_COMMAND_OPTIONS} --suppressions=${CMAKE_CURRENT_LIST_DIR}/valgrind.supp")
  endif()
  include (CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
