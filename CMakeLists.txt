project(input-event)
cmake_minimum_required(VERSION 3.14)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (BUILD_EXAMPLE)
    add_executable(${PROJECT_NAME}-example src/example/Example.cpp)
    target_include_directories(${PROJECT_NAME}-example PUBLIC include)
    target_link_libraries(${PROJECT_NAME}-example pthread)
endif()

if (BUILD_TEST)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

    CPMAddPackage(
        NAME turtle
        GITHUB_REPOSITORY mat007/turtle
        GIT_TAG v1.3.2
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/turtle
        DOWNLOAD_ONLY True
        GIT_SHALLOW True
    )

    CPMAddPackage(
        NAME catch2
        GITHUB_REPOSITORY catchorg/Catch2
        GIT_TAG v2.12.3
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/catch2
        DOWNLOAD_ONLY True
        GIT_SHALLOW True
    )

    add_subdirectory(src)
    add_subdirectory(test)
endif()
