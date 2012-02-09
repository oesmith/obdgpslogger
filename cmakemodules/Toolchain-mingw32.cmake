# Used for compiling Win32 binaries from Linux
# To use:
# 1) Install your distro's mingw32 package plus dependencies:
#     sudo aptitude install mingw32
#     yum install mingw32-\*
# 2) Make a build directory
#     cd {obdgpslogger-top-level-checkout}
#     mkdir build-mingw32
#     cd build-mingw32
# 3) Build
#     ccmake -DCMAKE_TOOLCHAIN_FILE=../cmakemodules/Toolchain-mingw32.cmake ..
#     make obdsim

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
SET(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH  /usr/i586-mingw32msvc /home/chunky/mingwlibs )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
