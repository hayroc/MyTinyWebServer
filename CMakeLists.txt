# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project(webserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")

aux_source_directory(. SERVER)
aux_source_directory(./base BASE)
aux_source_directory(./http HTTP)
aux_source_directory(./thread THREAD)
aux_source_directory(./timer TIMER)
aux_source_directory(./log LOGG)

# 指定生成目标
add_executable(webserver ${SERVER} ${BASE} ${HTTP} ${TIMER} ${THREAD} ${LOGG})
