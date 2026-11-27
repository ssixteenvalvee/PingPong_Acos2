cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

set(COMMON_SOURCES
	src/socket/socket.cpp
	src/history/history.cpp
)

add_executable(server
	src/server/main.cpp
	src/server/server.cpp
	${COMMON_SOURCES}
)

target_include_directories(server PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(client
	src/client/main.cpp
	src/client/client.cpp
	${COMMON_SOURCES}
)
target_include_directories(client PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
