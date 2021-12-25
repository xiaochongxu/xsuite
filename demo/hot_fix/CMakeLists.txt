# require cmake version.
cmake_minimum_required(VERSION 3.2)

# project name.
project(hot_fix)

# set path.
set(CUR_PATH .)
set(ROOT_PATH ${CUR_PATH}/../../)
set(COMMON_DIR ${ROOT_PATH}/common/)

# set flag.
add_compile_options(-g -O0 -fPIC)
add_link_options(-g -rdynamic -fPIC)

# set include directory.
include_directories(.)
include_directories(${COMMON_DIR})

# set source code file.
file(GLOB SERVICE_FILES s*.c)
file(GLOB PATCH_FILES p*.c)

# set target.
add_executable(${PROJECT_NAME} ${SERVICE_FILES})
add_library(${PROJECT_NAME}.patch SHARED ${PATCH_FILES})

target_link_libraries(${PROJECT_NAME} pthread dl)
