
# 为每个库定义依赖策略选项
option(SPDLOG_USE_SYSTEM "Use system-installed spdlog"      OFF)
option(SPDLOG_USE_LOCAL "Use local spdlog source"           OFF)
option(SPDLOG_USE_PACKAGED "Use pre-packaged spdlog"        OFF)
option(SPDLOG_USE_FETCH "Use FetchContent for spdlog"       ON)
option(SPDLOG_USE_SHARED "Use shared library for spdlog"                ON)

option(ASIO_USE_SYSTEM "Use system-installed asio"          OFF)
option(ASIO_USE_LOCAL "Use local asio source"               OFF)
option(ASIO_USE_PACKAGED "Use pre-packaged asio"            OFF)
option(ASIO_USE_FETCH "Use FetchContent for asio"           ON)

option(FMT_USE_SYSTEM "Use system-installed fmt"            OFF)
option(FMT_USE_LOCAL "Use local fmt source"                 OFF)
option(FMT_USE_PACKAGED "Use pre-packaged fmt"              OFF)
option(FMT_USE_FETCH "Use FetchContent for fmt"             ON)

option(UDPTCP_USE_SYSTEM "Use system-installed udp_tcp"     OFF)
option(UDPTCP_USE_LOCAL "Use local udp_tcp source"          OFF)
option(UDPTCP_USE_LOCAL "Use local udp_tcp source"          OFF)
option(UDPTCP_USE_FETCH "Use FetchContent for udp_tcp"      ON)
option(UDPTCP_USE_SHARED "Use shared library for tcp_udp_communicate"   ON)
