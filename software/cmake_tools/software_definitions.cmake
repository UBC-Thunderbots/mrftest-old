
# use pthread
add_definitions("-pthread")

# linker options
add_definitions("-Wl,--as-needed")
add_definitions("-Wl,-02")

# Include full debug information.
add_definitions("-ggdb3")

# use c++11
add_definitions("-std=c++11")

##### strerror_r #####
add_definitions("-D_POSIX_C_SOURCE=200112L")

##### toggle for 32/64 bit offsets #####
add_definitions("-D_FILE_OFFSET_BITS=64")

##### Define the [U]INT[N]_C macros in <cinttypes>#####
add_definitions("-D__STDC_CONSTANT_MACROS=1")

##### Use inline functions where available from GNU Scientific Library #####
add_definitions("-DHAVE_INLINE")

# Optimize reasonably well for the native machine.
add_definitions("-march=native -O2")

# Do not allow multiple definitions of the same global variable.
add_definitions("-fno-common")

# Optimize by assuming nobody checks errno for math functions.
add_definitions("-fno-math-errno")

# Optimize by assuming pointers do not alias when they are not permitted to do so according to type.
add_definitions("-fstrict-aliasing")
add_definitions("-Wall")

#add_definitions("-D_GLIBCXX_USE_C99_MATH=1")