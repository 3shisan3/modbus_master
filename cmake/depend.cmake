# git submodule 暂作记录保证稳定编译版本使用，使用 FetchContent 进行最新代码参与编译

# spdlog依赖管理 (支持指定编译生成动态库)
function(manage_spdlog)
    message(STATUS "Configuring spdlog dependency...")

    if(TARGET spdlog OR TARGET spdlog::spdlog)
        message(STATUS "  spdlog target already exists, skipping configuration")
        return()
    endif()
    
    if(SPDLOG_USE_SYSTEM)
        find_package(spdlog QUIET)
        if(spdlog_FOUND)
            message(STATUS "  Using system-installed spdlog")
            return()
        else()
            message(WARNING "System spdlog not found, falling back to next option")
        endif()
    endif()

    if(SPDLOG_USE_PACKAGED)
        if(EXISTS "${3RD_DIR}/spdlog/lib" AND EXISTS "${3RD_DIR}/spdlog/include")
            message(STATUS "  Using pre-packaged spdlog from ${3RD_DIR}/spdlog")
            
            # 根据动态库选项选择库文件 （windows下库文件是libspdlogd.dll, 需临时修改）
            if(SPDLOG_USE_SHARED AND EXISTS "${3RD_DIR}/spdlog/lib/${CMAKE_SHARED_LIBRARY_PREFIX}spdlog${CMAKE_SHARED_LIBRARY_SUFFIX}")
                add_library(spdlog_packaged SHARED IMPORTED)
                set_target_properties(spdlog_packaged PROPERTIES
                    IMPORTED_LOCATION "${3RD_DIR}/spdlog/lib/${CMAKE_SHARED_LIBRARY_PREFIX}spdlog${CMAKE_SHARED_LIBRARY_SUFFIX}"
                    INTERFACE_INCLUDE_DIRECTORIES "${3RD_DIR}/spdlog/include"
                )
            else()
                add_library(spdlog_packaged STATIC IMPORTED)
                set_target_properties(spdlog_packaged PROPERTIES
                    IMPORTED_LOCATION "${3RD_DIR}/spdlog/lib/${CMAKE_STATIC_LIBRARY_PREFIX}spdlog${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    INTERFACE_INCLUDE_DIRECTORIES "${3RD_DIR}/spdlog/include"
                )
            endif()
            
            add_library(spdlog::spdlog ALIAS spdlog_packaged)
            return()
        else()
            message(WARNING "Pre-packaged spdlog not found at ${3RD_DIR}/spdlog")
        endif()
    endif()

    if(SPDLOG_USE_LOCAL)
        if(EXISTS "${3DEPEND_DIR}/spdlog/CMakeLists.txt")
            message(STATUS "  Using local spdlog from ${3DEPEND_DIR}/spdlog")

            # 设置动态库选项传递给子项目
            set(SPDLOG_BUILD_SHARED ${SPDLOG_USE_SHARED})
            add_subdirectory("${3DEPEND_DIR}/spdlog" spdlog)
            return()
        else()
            message(WARNING "Local spdlog not found at ${3DEPEND_DIR}/spdlog")
        endif()
    endif()

    if(SPDLOG_USE_FETCH)
        message(STATUS "  Using FetchContent for spdlog")
        include(FetchContent)
        FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.13.0
        )

        # 设置动态库选项
        set(SPDLOG_BUILD_SHARED ${SPDLOG_USE_SHARED})
        FetchContent_MakeAvailable(spdlog)
        return()
    endif()

    message(FATAL_ERROR "No spdlog dependency strategy selected or available")
endfunction()

# asio依赖管理
function(manage_asio)
    message(STATUS "Configuring asio dependency...")
    
    if(ASIO_USE_SYSTEM)
        find_package(Asio QUIET)
        if(Asio_FOUND)
            message(STATUS "  Using system-installed asio")
            add_library(asio INTERFACE)
            target_include_directories(asio INTERFACE ${Asio_INCLUDE_DIR})
            target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
            return()
        else()
            message(WARNING "System asio not found, falling back to next option")
        endif()
    endif()

    if(ASIO_USE_PACKAGED)
        if(EXISTS "${3RD_DIR}/asio/include/asio.hpp")
            message(STATUS "  Using pre-packaged asio from ${3RD_DIR}/asio")
            add_library(asio INTERFACE)
            target_include_directories(asio INTERFACE "${3RD_DIR}/asio/include")
            target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
            return()
        else()
            message(WARNING "Pre-packaged asio not found at ${3RD_DIR}/asio")
        endif()
    endif()

    if(ASIO_USE_LOCAL)
        if(EXISTS "${3DEPEND_DIR}/asio/asio/include/asio.hpp")
            message(STATUS "  Using local asio from ${3DEPEND_DIR}/asio")
            add_library(asio INTERFACE)
            target_include_directories(asio INTERFACE "${3DEPEND_DIR}/asio/asio/include")
            target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
            return()
        else()
            message(WARNING "Local asio not found at ${3DEPEND_DIR}/asio")
        endif()
    endif()

    if(ASIO_USE_FETCH)
        message(STATUS "  Using FetchContent for asio")
        include(FetchContent)
        FetchContent_Declare(
            asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG asio-1-28-0
        )
        FetchContent_Populate(asio)
        
        add_library(asio INTERFACE)
        target_include_directories(asio INTERFACE "${asio_SOURCE_DIR}/asio/include")
        target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
        return()
    endif()

    message(FATAL_ERROR "No asio dependency strategy selected or available")
endfunction()

# fmt依赖管理
function(manage_fmt)
    message(STATUS "Configuring fmt dependency...")
    
    if(FMT_USE_SYSTEM)
        find_package(fmt QUIET)
        if(fmt_FOUND)
            message(STATUS "  Using system-installed fmt")
            return()
        else()
            message(WARNING "System fmt not found, falling back to next option")
        endif()
    endif()

    if(FMT_USE_PACKAGED)
        if(EXISTS "${3RD_DIR}/fmt/lib" AND EXISTS "${3RD_DIR}/fmt/include")
            message(STATUS "  Using pre-packaged fmt from ${3RD_DIR}/fmt")
            
            add_library(fmt_packaged STATIC IMPORTED)
            set_target_properties(fmt_packaged PROPERTIES
                IMPORTED_LOCATION "${3RD_DIR}/fmt/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt${CMAKE_STATIC_LIBRARY_SUFFIX}"
                INTERFACE_INCLUDE_DIRECTORIES "${3RD_DIR}/fmt/include"
            )
            
            # 创建别名以保持一致性
            add_library(fmt::fmt ALIAS fmt_packaged)
            return()
        else()
            message(WARNING "Pre-packaged fmt not found at ${3RD_DIR}/fmt")
        endif()
    endif()

    if(FMT_USE_LOCAL)
        if(EXISTS "${3DEPEND_DIR}/fmt/CMakeLists.txt")
            message(STATUS "  Using local fmt from ${3DEPEND_DIR}/fmt")
            add_subdirectory("${3DEPEND_DIR}/fmt" fmt)
            return()
        else()
            message(WARNING "Local fmt not found at ${3DEPEND_DIR}/fmt")
        endif()
    endif()

    if(FMT_USE_FETCH)
        message(STATUS "  Using FetchContent for fmt")
        include(FetchContent)
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.1.1
        )
        FetchContent_MakeAvailable(fmt)
        return()
    endif()

    message(FATAL_ERROR "No fmt dependency strategy selected or available")
endfunction()

# udp_tcp_communicate依赖管理
function(manage_udptcp)
    message(STATUS "Configuring udp-tcp-communicate dependency...")
    
    if(UDPTCP_USE_SYSTEM)
        find_package(udp-tcp-communicate QUIET)
        if(udp-tcp-communicate_FOUND)
            message(STATUS "  Using system-installed udp-tcp-communicate")
            return()
        else()
            message(WARNING "System udp-tcp-communicate not found, falling back to next option")
        endif()
    endif()

    if(UDPTCP_USE_PACKAGED)
        if(EXISTS "${3RD_DIR}/udp-tcp-communicate/lib" AND EXISTS "${3RD_DIR}/udp-tcp-communicate/include")
            message(STATUS "  Using pre-packaged udp-tcp-communicate from ${3RD_DIR}/udp-tcp-communicate")
            
            # 根据动态库选项选择库文件
            if(UDPTCP_SHARED AND EXISTS "${3RD_DIR}/udp-tcp-communicate/lib/${CMAKE_SHARED_LIBRARY_PREFIX}udp-tcp-communicate${CMAKE_SHARED_LIBRARY_SUFFIX}")
                add_library(udp-tcp-communicate_packaged SHARED IMPORTED)
                set_target_properties(udp-tcp-communicate_packaged PROPERTIES
                    IMPORTED_LOCATION "${3RD_DIR}/udp-tcp-communicate/lib/${CMAKE_SHARED_LIBRARY_PREFIX}udp-tcp-communicate${CMAKE_SHARED_LIBRARY_SUFFIX}"
                    INTERFACE_INCLUDE_DIRECTORIES "${3RD_DIR}/udp-tcp-communicate/include"
                )
            else()
                add_library(udp-tcp-communicate_packaged STATIC IMPORTED)
                set_target_properties(udp-tcp-communicate_packaged PROPERTIES
                    IMPORTED_LOCATION "${3RD_DIR}/udp-tcp-communicate/lib/${CMAKE_STATIC_LIBRARY_PREFIX}udp-tcp-communicate${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    INTERFACE_INCLUDE_DIRECTORIES "${3RD_DIR}/udp-tcp-communicate/include"
                )
            endif()
            
            add_library(udp-tcp-communicate::udp-tcp-communicate ALIAS udp-tcp-communicate_packaged)
            return()
        else()
            message(WARNING "Pre-packaged udp-tcp-communicate not found at ${3RD_DIR}/udp-tcp-communicate")
        endif()
    endif()

    if(UDPTCP_USE_LOCAL)
        if(EXISTS "${3DEPEND_DIR}/udp-tcp-communicate/CMakeLists.txt")
            message(STATUS "  Using local udp-tcp-communicate from ${3DEPEND_DIR}/udp-tcp-communicate")
            
            # 设置动态库选项传递给子项目
            set(UDPTCP_BUILD_SHARED ${UDPTCP_USE_SHARED} PARENT_SCOPE)
            set(SPDLOG_BUILD_SHARED ${SPDLOG_USE_SHARED})
            add_subdirectory("${3DEPEND_DIR}/udp-tcp-communicate" udp-tcp-communicate)
            return()
        else()
            message(WARNING "Local udp-tcp-communicate not found at ${3DEPEND_DIR}/udp-tcp-communicate")
        endif()
    endif()

    if(UDPTCP_USE_FETCH)
        message(STATUS "  Using FetchContent for udp-tcp-communicate")
        include(FetchContent)
        FetchContent_Declare(
            udp-tcp-communicate
            GIT_REPOSITORY https://github.com/3shisan3/udp-tcp-communicate.git
            GIT_TAG main
        )
        
        # 设置动态库选项
        set(BUILD_SHARED_LIBS ${UDPTCP_USE_SHARED})
        FetchContent_MakeAvailable(udp-tcp-communicate)
        return()
    endif()

    message(FATAL_ERROR "No udp-tcp-communicate dependency strategy selected or available")
endfunction()

# 验证依赖策略冲突 (应该保证唯一性，确保 option 中是互斥设置)
function(validate_dependency_strategy lib_name)
    set(use_system ${lib_name}_USE_SYSTEM)
    set(use_local ${lib_name}_USE_LOCAL)
    set(use_packaged ${lib_name}_USE_PACKAGED)
    set(use_fetch ${lib_name}_USE_FETCH)
    
    set(selected_count 0)
    if(${use_system})
        math(EXPR selected_count "${selected_count} + 1")
    endif()
    if(${use_local})
        math(EXPR selected_count "${selected_count} + 1")
    endif()
    if(${use_packaged})
        math(EXPR selected_count "${selected_count} + 1")
    endif()
    if(${use_fetch})
        math(EXPR selected_count "${selected_count} + 1")
    endif()
    
    if(selected_count GREATER 1)
        message(WARNING "Multiple strategies selected for ${lib_name}, will use in order: SYSTEM -> PACKAGED -> LOCAL -> FETCH")
    elseif(selected_count EQUAL 0)
        message(STATUS "No strategy selected for ${lib_name}, using default: FETCH")
        set(${lib_name}_USE_FETCH ON PARENT_SCOPE)
    endif()
endfunction()

# 在包含时验证所有策略
validate_dependency_strategy("SPDLOG")
validate_dependency_strategy("ASIO")
validate_dependency_strategy("FMT")
validate_dependency_strategy("UDPTCP")