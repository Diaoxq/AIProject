cmake_minimum_required(VERSION 3.10)
project(ChatLLM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# 可执行文件
add_executable(chatllm 
    src/main.cpp
)

# 头文件目录
target_include_directories(chatllm PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# 链接库
target_link_libraries(chatllm PRIVATE 
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# 包含当前目录的 nlohmann json 头文件
set(NLOHMANN_JSON_INCLUDE_DIRS "include")

# 安装配置
install(TARGETS chatllm DESTINATION bin)
