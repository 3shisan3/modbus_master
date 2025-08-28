cmake_minimum_required(VERSION 3.12)
project(modbus_master LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置路径变量
set(3DEPEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/depend)
set(3RD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/${SDK_PLATFORM})

# 包含目录
include_directories(
    ${SOURCE_CODE_DIR}/api
    ${SOURCE_CODE_DIR}/communicate
    ${SOURCE_CODE_DIR}/driver
    ${PLATFORM_HEADERS}
)

# 源码目录
set(SOURCE_CDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 收集项目源文件
file(GLOB_RECURSE PROJECT_SRCS
    ${SOURCE_CODE_DIR}/api/*.cpp
    ${SOURCE_CODE_DIR}/communicate/*.cpp
    ${SOURCE_CODE_DIR}/driver/*.cpp
)

# # 创建可执行文件
# add_executable(${PROJECT_NAME}
#     ${PROJECT_SRCS}
#     ${PLATFORM_SOURCES}
#     ${SOURCE_CODE_DIR}/main.cpp)

# # 链接库
# target_link_libraries(${PROJECT_NAME} PRIVATE
#     ${SPDLOG_LIBRARY}
#     ${TCP_UDP_COMM_LIBRARY}
#     fmt::fmt)

