if (GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
    set(TEST_LIBRARIES ${GTEST_BOTH_LIBRARIES})
else()
    string(TOLOWER  ${CMAKE_BUILD_TYPE} BUILD_TYPE)
    if (${BUILD_TYPE} STREQUAL "debug")
        message("Googletest ${TEST} DEBUG MODE: ${CMAKE_BUILD_TYPE}")
        set(TEST_LIBRARIES gtestd;gtest_maind)
    else()
        message("Googletest ${TEST} RELEASE MODE: ${CMAKE_BUILD_TYPE}")
        set(TEST_LIBRARIES gtest;gtest_main)
    endif()
endif()

if (NOT WIN32)
    set(TEST_LIBRARIES ${TEST_LIBRARIES};pthread)
endif ()


file (GLOB TESTS_SOURCES ${TESTS_SOURCES}
        ./*.cpp
        )


file (GLOB TESTS_SOURCES ${TESTS_SOURCES}
        ./*.cpp
        )

set (TEST digest_${PROJECT_LIB})

add_executable(${TEST} ${TESTS_SOURCES})


if (COMMON_DEPENDENCIES)
    message(STATUS "${TEST} DEPENDENCIES: ${COMMON_DEPENDENCIES}")
    add_dependencies(
            ${TEST}
            ${COMMON_DEPENDENCIES}
    )
endif ()

target_link_libraries (
        ${TEST}
        ${PROJECT_LIB}
        ${TEST_LIBRARIES})

add_test (test ${TEST})
enable_testing ()
