cmake_minimum_required(VERSION 3.24)
project(TryWebServer)

set(CMAKE_CXX_STANDARD 14)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(TryWebServer code/main.cpp
        code/http/http_response.cpp
        code/http/http_conn.cpp
        code/http/http_request.cpp
        code/server/epoller.cpp
        code/timer/timer.cpp
        code/log/log.cpp
        code/pool/sql_conn_pool.cpp
        code/buffer/buffer.cpp
        code/server/web_server.cpp)

target_link_libraries(TryWebServer libmysqlclient.so)