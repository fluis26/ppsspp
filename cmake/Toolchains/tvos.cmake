# Options:
#
# IOS_PLATFORM = OS (default)
#   This needs to be OS as the simulator cannot use the required GLES2.0 environment.
#   OS - the default, used to build for iPhone and iPad physical devices, which have an arm arch.
#
# CMAKE_TVOS_SDK_ROOT = automatic(default) or /path/to/platform/Developer/SDKs/SDK folder
#   By default this location is automatcially chosen based on the CMAKE_IOS_DEVELOPER_ROOT value.
#   In this case it will always be the most up-to-date SDK found in the CMAKE_IOS_DEVELOPER_ROOT path.
#   If set manually, this will force the use of a specific SDK version

# PPSSPP platform flags
set(MOBILE_DEVICE ON)
set(USING_GLES2 ON)
set(TVOS_DEPLOYMENT_TARGET 11.4)
add_definitions(
  -DGL_ETC1_RGB8_OES=0
  -U__STRICT_ANSI__
)

set(OPENGL_LIBRARIES "-framework OpenGLES")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtvos-version-min=${TVOS_DEPLOYMENT_TARGET}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtvos-version-min=${TVOS_DEPLOYMENT_TARGET}")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

# Standard settings
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(IOS ON)
set(TVOS ON)
set(CMAKE_CROSSCOMPILING ON)
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET ${TVOS_DEPLOYMENT_TARGET})

set(IOS_PLATFORM "TVOS")
set(IOS_PLATFORM ${IOS_PLATFORM} CACHE STRING "Type of OS Platform")

# Check the platform selection and setup for developer root
if(IOS_PLATFORM STREQUAL "TVOS")
  set(IOS_SDK_NAME "appletvos")
elseif(IOS_PLATFORM STREQUAL "SIMULATOR_TVOS")
  set(IOS_SDK_NAME "appletvsimulator")
else()
  message (FATAL_ERROR "Unsupported IOS_PLATFORM value selected '${IOS_PLATFORM}'. Please choose OS or leave default")
endif()

# Setup iOS developer location unless specified manually with CMAKE_TVOS_DEVELOPER_ROOT
if(NOT CMAKE_TVOS_SDK_ROOT)
  execute_process(COMMAND xcrun --sdk ${IOS_SDK_NAME} --show-sdk-path
    OUTPUT_VARIABLE CMAKE_TVOS_SDK_ROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message (STATUS "Toolchain using default tvOS SDK: ${CMAKE_TVOS_SDK_ROOT}")
endif()

set(CMAKE_TVOS_SDK_ROOT ${CMAKE_TVOS_SDK_ROOT} CACHE PATH "Location of the selected tvOS SDK")

# Set the sysroot default to the most recent SDK
set(CMAKE_OSX_SYSROOT ${CMAKE_TVOS_SDK_ROOT} CACHE PATH "Sysroot used for tvOS support")

# set the architecture for tvOS 
if(IOS_PLATFORM STREQUAL "TVOS")
  set(IOS_ARCH "arm64")
else()
  set(IOS_ARCH "i386;x86_64")
endif()

set(CMAKE_OSX_ARCHITECTURES "${IOS_ARCH}" CACHE STRING "Build architecture for tvOS")
set(CMAKE_ASM_FLAGS "" CACHE STRING "" FORCE)
foreach(arch ${IOS_ARCH})
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch ${arch}" CACHE STRING "" FORCE)
endforeach()

# Set the find root to the iOS developer roots and to user defined paths
set(CMAKE_FIND_ROOT_PATH ${CMAKE_TVOS_SDK_ROOT} ${CMAKE_PREFIX_PATH} CACHE STRING "tvOS find search path root")

# default to searching for frameworks first
set(CMAKE_FIND_FRAMEWORK FIRST)

# set up the default search directories for frameworks
set(CMAKE_SYSTEM_FRAMEWORK_PATH
  ${CMAKE_TVOS_SDK_ROOT}/System/Library/Frameworks
  ${CMAKE_TVOS_SDK_ROOT}/System/Library/PrivateFrameworks
  ${CMAKE_TVOS_SDK_ROOT}/Developer/Library/Frameworks
)

# only search the tvOS sdks, not the remainder of the host filesystem
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

