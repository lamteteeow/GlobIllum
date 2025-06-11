cmake_minimum_required(VERSION 3.16)
project(gi)

# ---------------------------------------------------------------------
# options

option(GI_STAT_COUNTERS "Enable stat collection during rendering (very slow)." OFF)

# ---------------------------------------------------------------------
# path management

# look for "deps" folder in root or parent directories
set(DEPDIR "${CMAKE_SOURCE_DIR}/deps")
if(NOT EXISTS ${DEPDIR})
    set(DEPDIR "${CMAKE_SOURCE_DIR}/../deps")
elseif(NOT EXISTS ${DEPDIR})
    message(FATAL_ERROR "Dependency directory not found, please download and unzip from StudOn!")
endif()

# look for "data" folder in root or parent directories
set(DATADIR "${CMAKE_SOURCE_DIR}/data")
if(NOT EXISTS ${DATADIR})
    set(DATADIR "${CMAKE_SOURCE_DIR}/../data")
elseif(NOT EXISTS ${DATADIR})
    message(FATAL_ERROR "Data directory not found, please download and unzip from StudOn!")
endif()

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

message(STATUS "BINDIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "DEPDIR: ${DEPDIR}")
message(STATUS "DATADIR: ${DATADIR}")
message(STATUS "CONFDIR: ${CMAKE_SOURCE_DIR}/configs")

# ---------------------------------------------------------------------
# dependencies

# prefer static libs
set(BUILD_SHARED_LIBS OFF)

# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(EMSCRIPTEN)
    add_compile_options(-pthread)
endif()

# OpenMP
if(EMSCRIPTEN)
    include(cmake/GitCheckout.cmake)
    git_checkout(
        URL "https://github.com/llvm/llvm-project.git"
        REF llvmorg-20.1.0
        SPARSE openmp cmake
    )
    set(OPENMP_STANDALONE_BUILD ON CACHE BOOL "")
    set(OPENMP_ENABLE_OMPT_TOOLS OFF CACHE BOOL "")
    set(OPENMP_ENABLE_LIBOMPTARGET OFF CACHE BOOL "")
    set(LIBOMP_HAVE_OMPT_SUPPORT OFF CACHE BOOL "")
    set(LIBOMP_OMPT_SUPPORT OFF CACHE BOOL "")
    set(LIBOMP_OMPD_SUPPORT OFF CACHE BOOL "")
    set(LIBOMP_USE_DEBUGGER OFF CACHE BOOL "")
    set(LIBOMP_FORTRAN_MODULES OFF CACHE BOOL "")
    set(LIBOMP_ENABLE_SHARED OFF CACHE BOOL "")
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/llvm-project/openmp openmp)
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/openmp/runtime/src)
    target_compile_options(omp PUBLIC -Wno-unused-command-line-argument)
else()
    find_package(OpenMP REQUIRED)
    include_directories(${OpenMP_CXX_INCLUDE_DIRS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# GLEW
if(EMSCRIPTEN)
    # included
else()
    set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    add_subdirectory(${DEPDIR}/glew-cmake ${CMAKE_CURRENT_BINARY_DIR}/glew)
    include_directories(${DEPDIR}/glew-cmake/include)
endif()

# GLFW
if(EMSCRIPTEN)
    # included
else()
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
    set(GLFW_USE_CHDIR OFF CACHE BOOL " " FORCE)
    add_subdirectory(${DEPDIR}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
    include_directories(${DEPDIR}/glfw/include)
endif()

# OpenGL
if(EMSCRIPTEN)
    # included
else()
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIR})
endif()

# GLM
include_directories(${DEPDIR}/glm)

# Assimp
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_EXPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "")
add_subdirectory(${DEPDIR}/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp)
include_directories(${DEPDIR}/assimp/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/assimp/include)
if(EMSCRIPTEN)
    # see https://github.com/assimp/assimp/issues/5908
    target_compile_options(assimp PUBLIC -Wno-nontrivial-memaccess)
    target_compile_options(zlibstatic PUBLIC -Wno-deprecated-non-prototype)
endif()

# Embree
set(EMBREE_TASKING_SYSTEM "INTERNAL" CACHE STRING " " FORCE)
set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL " " FORCE)
set(EMBREE_STATIC_LIB ON CACHE BOOL " " FORCE)
set(EMBREE_TUTORIALS OFF CACHE BOOL " " FORCE)
set(EMBREE_GEOMETRY_CURVE OFF CACHE BOOL " " FORCE)
set(EMBREE_GEOMETRY_GRID OFF CACHE BOOL " " FORCE)
set(EMBREE_GEOMETRY_POINT OFF CACHE BOOL " " FORCE)
set(EMBREE_GEOMETRY_SUBDIVISION OFF CACHE BOOL " " FORCE)
if(EMSCRIPTEN)
    set(FLAGS_SSE2 "-msse -msse2")
    set(EMBREE_MAX_ISA SSE2 CACHE STRING " " FORCE)
else()
    set(EMBREE_MAX_ISA AVX2 CACHE STRING " " FORCE)
endif()
add_subdirectory(${DEPDIR}/embree ${CMAKE_CURRENT_BINARY_DIR}/embree)
include_directories(${DEPDIR}/embree/include)

# Open Image Denoise
find_package(OpenImageDenoise QUIET)
if(NOT OpenImageDenoise_FOUND)
    message(STATUS "OIDN NOT FOUND: compiling without denoiser")
else()
    message(STATUS "OIDN FOUND: ${OpenImageDenoise_DIR}")
    include_directories(${OpenImageDenoise_INCLUDE_DIRS})
endif()

# nanoflann
include_directories(${DEPDIR}/nanoflann)

# stb image
include_directories(${DEPDIR}/stb)

# ---------------------------------------------------------------------
# compiler options

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

if(EMSCRIPTEN)
    add_definitions(-DGI_DATA_DIR="/data")
    add_definitions(-DGI_CONF_DIR="/configs")
else()
    add_definitions(-DGI_DATA_DIR="${DATADIR}")
    add_definitions(-DGI_CONF_DIR="${CMAKE_SOURCE_DIR}/configs")
endif()

if(GI_STAT_COUNTERS)
    add_definitions(-DSTATS)
else()
    add_definitions(-USTATS)
endif()

if(OpenImageDenoise_FOUND)
    add_definitions(-DWITH_OIDN)
else()
    add_definitions(-UWITH_OIDN)
endif()

if(UNIX AND NOT EMSCRIPTEN)
    add_compile_options(-march=native -ffast-math)
elseif(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
endif()

# ---------------------------------------------------------------------
# set compilation target

set(TARGET gi)

# glob source files
include_directories(src)
file(GLOB_RECURSE SOURCES "src/*.cpp")

# define target
add_executable(${TARGET} ${SOURCES})

# ----------------------------------------------------------
# link dependencies

target_link_libraries(${TARGET} embree assimp)
if(OpenImageDenoise_FOUND)
    target_link_libraries(${TARGET} OpenImageDenoise)
endif()

if(EMSCRIPTEN)
    target_link_libraries(${TARGET} omp)
else()
    target_link_libraries(${TARGET} Threads::Threads OpenMP::OpenMP_CXX)
    target_link_libraries(${TARGET} ${OPENGL_LIBRARIES} libglew_static glfw)
endif()

# ----------------------------------------------------------
# finalize emscripten config

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_compile_options(${TARGET} PRIVATE
        "-msimd128" "-msse" "-msse2" "-msse3"
        "-Wno-nontrivial-memaccess" 
        "-fopenmp"
    )
    target_link_options(${TARGET} PRIVATE
        "-pthread"
        "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency"
        "-sPTHREAD_POOL_SIZE_STRICT=0"
        "-sALLOW_MEMORY_GROWTH=1"
        "--no-heap-copy"
        "-sUSE_GLFW=3"
        "-sFULL_ES3"
    )
    target_link_options(${TARGET} PRIVATE
        "--preload-file=${CMAKE_SOURCE_DIR}/configs@configs"
        "--preload-file=${DATADIR}@data"
        "--shell-file=${CMAKE_SOURCE_DIR}/emscripten/shell.html"
    )
endif()
