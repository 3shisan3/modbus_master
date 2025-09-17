cmake_minimum_required(VERSION 3.12)
project(modbus_master LANGUAGES CXX)

# 设置构建选项前提
option(MODBUS_BUILD_SHARED "Build shared library" ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/option.cmake)

# 设置路径变量
set(3DEPEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/depend)
set(3RD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/${SDK_PLATFORM})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # 强制所有目标编译为PIC
# set(CMAKE_BUILD_TYPE "Release")

# 添加编译输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 源码目录
set(SOURCE_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform.cmake)

# 包含目录
include_directories(
    ${SOURCE_CODE_DIR}/communicate
    ${SOURCE_CODE_DIR}/driver
    ${PLATFORM_HEADERS}
)

# 收集项目源文件
file(GLOB_RECURSE PROJECT_SRCS
    ${SOURCE_CODE_DIR}/communicate/*.cpp
    ${SOURCE_CODE_DIR}/driver/*.cpp
    ${PLATFORM_SOURCES}
)

# 补充依赖库内容
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/depend.cmake)
manage_udptcp()
manage_spdlog()
manage_asio()
manage_fmt()

# 编译生成库
if(MODBUS_BUILD_SHARED OR BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${PROJECT_SRCS})
else()
    add_library(${PROJECT_NAME} STATIC ${PROJECT_SRCS})
endif()

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog
    udp-tcp-communicate
    fmt::fmt
)

