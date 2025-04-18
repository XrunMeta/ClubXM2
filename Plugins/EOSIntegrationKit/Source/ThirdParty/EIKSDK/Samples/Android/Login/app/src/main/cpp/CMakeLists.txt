# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

set(BASE_PROJECT_DIR ${CMAKE_SOURCE_DIR}/../../../../../../..)
set(EOSSDK_DIR ${BASE_PROJECT_DIR}/SDK/Bin/Android/static-stdc++/aar/)

add_library(EOSSDK SHARED IMPORTED)
set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../../build/unpacked/eossdk-StaticSTDC-release/jni/${ANDROID_ABI}/libEOSSDK.so)
set_property(TARGET EOSSDK PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${BASE_PROJECT_DIR}/SDK/Include)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib
        #Links the target library to the EOSSDK
        EOSSDK

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )