# ASIO配置
if(NOT USE_SYSTEM_ASIO)
    set(ASIO_INCLUDE_DIRS ${3DEPEND_DIR}/asio/asio/include)
endif()

# FMT配置
if(NOT USE_SYSTEM_FMT)
    add_subdirectory(${3DEPEND_DIR}/fmt)
endif()

# 直接指定预编译库路径（根据实际文件名及平台调整）