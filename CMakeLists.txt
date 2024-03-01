cmake_minimum_required(VERSION 3.20)
project(h264decoder)



#change default build type to Release (for speed) rather than Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
            "Define build type, options are: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)


if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CXX_OPTIMIZATION_FLAG "-O0 -g")
else()
    set(CXX_OPTIMIZATION_FLAG "-O3")
endif()

if (NOT WIN32)
  # set defaults CPP flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format -fomit-frame-pointer -fstrict-aliasing")
endif()

# minor optimizations on x64 for GCC variants
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -malign-double -fprefetch-loop-arrays")
endif(CMAKE_COMPILER_IS_GNUCXX)


set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set (BuildExamples False CACHE BOOL "Build the examples")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
if (NOT WIN32)
        message(STATUS, "Not Win32")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wno-deprecated-declarations")
endif()

# Not working on Windows
#   find_package(PkgConfig REQUIRED)
#   pkg_check_modules(libswscale REQUIRED libswscale)
#   pkg_check_modules(libavutil REQUIRED libavutil)
#   pkg_check_modules(libavcodec REQUIRED libavcodec)
#
# Not on Ubuntu ...
# find_package(FFMPEG REQUIRED swscale avutil avcodec)

# So we do it more low level
message(STATUS, "CMAKE_SYSTEM is ${CMAKE_SYSTEM}")
message(STATUS, "CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")
message(STATUS, "CMAKE_HOST_SYSTEM_NAME is ${CMAKE_HOST_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    cmake_path(CONVERT "$ENV{HOMEPATH}" TO_CMAKE_PATH_LIST NEWPATH) 
    message(STATUS, "HOMEPATH IS $ENV{HOMEPATH}")
    message(STATUS, "NEWPATH IS ${NEWPATH}")
    set(HOME "$ENV{HOMEDRIVE}${NEWPATH}")
    message(STATUS, "HOME IS ${HOME}")
    string(COMPARE EQUAL "${CMAKE_TOOLCHAIN_FILE}" "" TOOLCHAIN_UNDEFINED)
    if (TOOLCHAIN_UNDEFINED)
        string(COMPARE EQUAL "$ENV{VCPKG_TOOLCHAIN}" "" VCPKG_ENV_UNDEFINED)
	if (VCPKG_ENV_UNDEFINED)
            # guess
	    set(CMAKE_TOOLCHAIN_FILE "${HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake") 
            if (NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
                message(FATAL_ERROR "Cannot find CMake toolchain file, and VCPKG_TOOLCHAIN environment variable is not set.")
            endif()
	else()
            set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_TOOLCHAIN}")
            if (NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
		    message(FATAL_ERROR "VCPKG_TOOLCHAIN environment variable is defined as $ENV{VCPKG_TOOLCHAIN}, but I cannot find a file there.")
            endif()
        endif()
    else()
        if (NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
		message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is defined as ${CMAKE_TOOLCHAIN_FILE}, but I cannot find a file there.")
        endif()
    endif()


    #set(FFMPEG_DIR "${HOME}\\vcpkg\\packages\\ffmpeg_x64-windows\\share\\ffmpeg")
    #set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${HOME}\\vcpkg\\packages")
    #set(CMAKE_MODULE_PATH "${HOME}/vcpkg/packages/ffmpeg_x64-windows/share/ffmpeg")

    set(FFMPEG_DIR "${HOME}/vcpkg/packages/ffmpeg_x64-windows/lib/pkgconfig")
    message("check in ${CMAKE_MODULE_PATH}")
    #  message("check in ${FFMPEG_DIR}")
    # must include the toolchain file now, to get the find_package to work.
    include (${CMAKE_TOOLCHAIN_FILE})
    message("CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
    find_package(FFMPEG REQUIRED  )

    include_directories(${FFMPEG_INCLUDE_DIRS})
    message(STATUS, "FFMPEG_INCLUDE_DIRS are ${FFMPEG_INCLUDE_DIRS}")
    message(STATUS, "FFMPEG_LIBRARY_DIRS are ${FFMPEG_LIBRARY_DIRS}")
    message(STATUS, "FFMPEG_LIBRARIES are ${FFMPEG_LIBRARIES}")


elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
       # MacOS
    find_path( AVCODEC_INCLUDE_DIR libavcodec/avcodec.h )
    find_library( AVCODEC_LIBRARY avcodec )
    find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h )
    find_library( AVUTIL_LIBRARY avutil )
    find_path( SWSCALE_INCLUDE_DIR libswscale/swscale.h )
    find_library( SWSCALE_LIBRARY swscale )
set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
set(FFMPEG_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR})

else ()
    find_path( AVCODEC_INCLUDE_DIR libavcodec/avcodec.h )
    find_library( AVCODEC_LIBRARY avcodec )
    find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h )
    find_library( AVUTIL_LIBRARY avutil )
    find_path( SWSCALE_INCLUDE_DIR libswscale/swscale.h )
    find_library( SWSCALE_LIBRARY swscale )
set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
set(FFMPEG_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR})


endif()	




find_package(pybind11)
if(pybind11_FOUND)
  message("Using existing pybind11 v${pybind11_VERSION}")
else()
  message("Fetching pybind11")
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    # Recent PyBind11 version is required for Python 3.11 support.
    GIT_TAG v2.10.2)
  FetchContent_MakeAvailable(pybind11)
endif()


add_library(h264decoderlib STATIC src/h264decoder.cpp src/h264decoder.hpp)
target_sources(h264decoderlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/h264decoder.hpp)
target_link_libraries(h264decoderlib PUBLIC ${FFMPEG_LIBRARIES})
target_include_directories(h264decoderlib PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_include_directories(h264decoderlib PUBLIC "src")

pybind11_add_module(h264decoder src/h264decoder_python.cpp)
target_link_libraries(h264decoder PRIVATE h264decoderlib)

if (BuildExamples)
  add_subdirectory(examples)
endif ()
