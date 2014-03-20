cmake_minimum_required(VERSION 2.6)

project(cutsat CXX)
enable_testing()

# Default is release with debug info
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

# If not in debug build add NDEBUG by default
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "Enabling tracing.")
    set(CMAKE_CXX_FLAGS "-DCUTSAT_TRACING_ENABLED")
    set(cutsat_TRACING ON)
endif()

# Add the target for the check
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

# If building a static binary
if(BUILD_STATIC_EXECUTABLES)
    message(STATUS "Building static binaries.")
    set(CMAKE_EXE_LINKER_FLAGS -static)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
    set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
    set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
    # Maybe this works as well, haven't tried yet.
    set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
    set(Boost_USE_STATIC_LIBS ON)
else()
    message(STATUS "Building dynamic binaries.")
    set(Boost_USE_STATIC_LIBS OFF)
    set(ENABLE_UNIT_TESTS ON)
endif()

# Find the Google-perf tools libraries
set(ADDITIONAL_LIBRARIES "")
if(GOOGLE_PERFTOOLS)
    find_library(TCMALLOC_LIBRARY tcmalloc)
    if (TCMALLOC_LIBRARY)
        message(STATUS "Adding additional: "${TCMALLOC_LIBRARY})
        set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${TCMALLOC_LIBRARY}) 
    endif()
    find_library(PROFILER_LIBRARY profiler)
    if (PROFILER_LIBRARY)
        message(STATUS "Adding additional: "${PROFILER_LIBRARY})
        set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${PROFILER_LIBRARY})
    endif()
endif()

# Find the Boost libraries
set(Boost_USE_MULTITHREADED OFF)
if (cutsat_TRACING)
    find_package(Boost 1.36.0 COMPONENTS program_options regex REQUIRED)
else()
    find_package(Boost 1.36.0 COMPONENTS program_options REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})

# For some reason we need to add these when building statically
if(BUILD_STATIC_EXECUTABLES)
    # Find the zip libraries
    find_library(Z_LIBRARY z)  
    if (Z_LIBRARY)
        message(STATUS "libz: "${Z_LIBRARY})
    else()
        MESSAGE(FATAL_ERROR "Could not the z library")
    endif()

    find_library(BZ2_LIBRARY bz2)
    if (BZ2_LIBRARY)
        message(STATUS "libbz2: "${BZ2_LIBRARY})
    else()
        MESSAGE(FATAL_ERROR "Could not the bz2 library (sudo apt-get install libbz2-dev)")
    endif()

    # Find the pthreads library (for some reason boost needs it)
    find_library(PTHREADS_LIBRARY pthread)  
    if (PTHREADS_LIBRARY)
        message(STATUS "libpthread: "${PTHREADS_LIBRARY})
    else()
        MESSAGE(FATAL_ERROR "Could not the pthreads library")
    endif()

    # Find the GMP number library
    find_library(GMP_LIBRARY gmp)
    if (GMP_LIBRARY)
        message(STATUS "GMP library: "${GMP_LIBRARY})
    else()
        MESSAGE(FATAL_ERROR "Could not the GMP number library (sudo apt-get libgmp3c2)")
    endif()
else()
    set(Z_LIBRARY "") 
    set(BZ2_LIBRARY "") 
    set(PTHREADS_LIBRARY "")
    set(GMP_LIBRARY "")
endif()

# Find the CLN number library
find_path(CLN_INCLUDE_DIR cln/integer.h)
find_library(CLN_LIBRARY cln)
if (CLN_INCLUDE_DIR AND CLN_LIBRARY)
    message(STATUS "CLN headers: "${CLN_INCLUDE_DIR})
    message(STATUS "CLN library: "${CLN_LIBRARY})
    include_directories(${CLN_INCLUDE_DIR})
else()
    MESSAGE(FATAL_ERROR "Could not the CLN number library (sudo apt-get libcln-dev)")
endif()

# Find the CoinUtils library
find_path(COIN_INCLUDE_DIR coin/CoinMpsIO.hpp)
find_library(COIN_LIBRARY CoinUtils)
if (COIN_INCLUDE_DIR AND COIN_LIBRARY)
    message(STATUS "CoinUtils headers: "${COIN_INCLUDE_DIR})
    message(STATUS "CoinUtils library: "${COIN_LIBRARY})
    include_directories(${COIN_INCLUDE_DIR})
else()
    MESSAGE(FATAL_ERROR "Could not find the CoinUtils library (sudo apt-get coinor-libcoinutils-dev)")
endif()

# Add our sources to the include
include_directories(${cutsat_SOURCE_DIR}/src)

# Add all the tests
file(STRINGS test/regress/regression_tests.txt REGRESSION_TESTS)

# Configure the subdirectories
add_subdirectory(src)

# Add the documentation
add_subdirectory(doc)

# Configure the packaging
execute_process(COMMAND svn info COMMAND grep Revision COMMAND sed s/Revision:\ // OUTPUT_VARIABLE SVN_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)

SET(CPACK_PACKAGE_NAME "cutsat")
set(CPACK_PACKAGE_CONTACT "Dejan Jovanovic <dejan@cs.nyu.edu>")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cutsat is a feasibility solver for problems in linear integer arithmetic.")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "${SVN_REVISION}")

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "cutsat-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES "/build/;/CVS/;/\\\\.svn/;/\\\\.bzr/;/\\\\.hg/;/\\\\.git/;\\\\.swp$;\\\\.#;/#;.*~;cscope.*;.*tmp")

INCLUDE(CPack)

# Add the test project
if (NOT BUILD_STATIC_EXECUTABLES)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_TEST_DYN_LINK")
endif()

add_subdirectory(test)
