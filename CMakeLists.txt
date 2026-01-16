cmake_minimum_required(VERSION 3.25)
project(dashsrv LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -O2)

include_directories(
    vendor/mongoose
    vendor
    include
)

set(SOURCES
    source/Dashsrv.cpp
    source/Basic.cpp
    source/MGClient.cpp
    source/Hardware.cpp
    source/Server/ServiceHandler.cpp
    source/Server/Routes.cpp
    source/Server/Core/HTTP.cpp
    source/Server/Core/Rand.cpp
    source/Server/Core/Server.cpp
    source/Server/Config.cpp
    source/Minecraft/MCPacket.cpp
    source/Minecraft/MCQuery.cpp
    source/Minecraft/Status.cpp
    source/Minecraft/String.cpp
    vendor/mongoose/mongoose.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 iphlpapi)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0600)
endif()
