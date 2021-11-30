cmake_minimum_required(VERSION 3.10)

project(xsfs)


# using pkg-config to find the apt fuse devel paths
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFUSE REQUIRED fuse3)

add_subdirectory(ouroboroslib EXCLUDE_FROM_ALL)

add_executable(xsfs xsfs.cpp)
add_executable(client client.cpp)
target_compile_features(xsfs PUBLIC cxx_std_17)

target_link_libraries(xsfs PUBLIC ${LIBFUSE_LIBRARIES} ouroboros "numa")
target_include_directories(xsfs PUBLIC ${LIBFUSE_INCLUDE_DIRS})
target_compile_options(xsfs PUBLIC ${LIBFUSE_CFLAGS_OTHER})

