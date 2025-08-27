# Windows平台特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    
    # 复制DLL到输出目录（如果是动态库）
    if(SPDLOG_LIBRARY MATCHES ".dll$" OR TCP_UDP_COMM_LIBRARY MATCHES ".dll$")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SPDLOG_LIBRARY}
            ${TCP_UDP_COMM_LIBRARY}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying DLLs to output directory")
    endif()
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${SOURCE_CODE_DIR}/include/ DESTINATION include)
install(FILES ${3RD_DIR}/include/tcp_udp_communicate.h DESTINATION include)
install(DIRECTORY ${SPDLOG_INCLUDE_DIR}/ DESTINATION include/spdlog)

# 打包配置
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modbus Master Application")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ModbusMaster")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Modbus Master")
    set(CPACK_NSIS_PACKAGE_NAME "Modbus Master")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
endif()

# 包含运行时库到打包中
if(EXISTS ${SPDLOG_LIBRARY})
    install(FILES ${SPDLOG_LIBRARY} DESTINATION bin)
endif()

if(EXISTS ${TCP_UDP_COMM_LIBRARY})
    install(FILES ${TCP_UDP_COMM_LIBRARY} DESTINATION bin)
endif()

include(CPack)