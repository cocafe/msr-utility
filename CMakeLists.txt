cmake_minimum_required(VERSION 3.19)

set(PROJECT_NAME msr-cmd)

project(${PROJECT_NAME} C)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_C_STANDARD 11)

#set(LIBJJ_HAVE_CJSON 1)
#set(LIBJJ_HAVE_SOCKET 1)
set(LIBJJ_HAVE_ICONV 1)
#set(LIBJJ_HAVE_MATH 1)
#set(LIBJJ_HAVE_UUID 1)

set(WIN32_UNICODE true)
set(WIN32_LINK_SUBSYS console)

set(APPRES_OBJS)
if (WIN32 AND MINGW)
        set(WINRES_OUT ${CMAKE_BINARY_DIR}/appres.o)

        add_custom_command(
                OUTPUT ${WINRES_OUT}
                COMMAND windres ${CMAKE_SOURCE_DIR}/asset/app.rc ${WINRES_OUT}
        )

        add_custom_target(WinResObj ALL DEPENDS ${WINRES_OUT})

        set(APPRES_OBJS ${WINRES_OUT})
endif()

add_executable(${PROJECT_NAME} ${APPRES_OBJS})

set(SOURCE_FILES
    asset/resource.h
    src/main.c
    )

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

if (WIN32 AND MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -static)
        target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc)

        if (WIN32_UNICODE)
                target_compile_options(${PROJECT_NAME} PRIVATE -municode)
                target_compile_definitions(${PROJECT_NAME} PRIVATE -DUNICODE)
                target_compile_definitions(${PROJECT_NAME} PRIVATE -D_UNICODE)
                target_link_options(${PROJECT_NAME} PRIVATE -municode)

                set(LIBJJ_WIN32_UNICODE 1)
        endif()

        if (${WIN32_LINK_SUBSYS} STREQUAL "window")
                target_compile_options(${PROJECT_NAME} PRIVATE -mwindows)
                target_compile_definitions(${PROJECT_NAME} PRIVATE -DSUBSYS_WINDOW)
                target_link_options(${PROJECT_NAME} PRIVATE -mwindows)

                set(LIBJJ_WIN32_WINDOW 1)
        endif()

        if (${WIN32_LINK_SUBSYS} STREQUAL "console")
                target_compile_options(${PROJECT_NAME} PRIVATE -mconsole)
                target_link_options(${PROJECT_NAME} PRIVATE -mconsole)
        endif()
endif()

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_compile_options(${PROJECT_NAME} PRIVATE -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-parameter)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-function)
target_compile_options(${PROJECT_NAME} PRIVATE -D_WIN32_WINNT=_WIN32_WINNT_WIN10)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -g3)
        target_compile_options(${PROJECT_NAME} PRIVATE -ggdb3)
        target_compile_options(${PROJECT_NAME} PRIVATE -gdwarf-4)
        target_compile_options(${PROJECT_NAME} PRIVATE -O0)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O2)
endif()

add_subdirectory(lib/libjj)
add_subdirectory(lib/libwinring0)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC lib)
target_include_directories(${PROJECT_NAME} PUBLIC asset)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} jjcom)
target_link_libraries(${PROJECT_NAME} winring0)
target_link_libraries(${PROJECT_NAME} ntdll)
target_link_libraries(${PROJECT_NAME} ntoskrnl)

set(INSTALL_DEST "Build-${CMAKE_BUILD_TYPE}")

install(TARGETS ${PROJECT_NAME} DESTINATION "${INSTALL_DEST}")

add_custom_target(collect_dll
                  COMMAND ${MSYS2_HOME}/usr/bin/bash.exe "${PROJECT_SOURCE_DIR}/collect_dll.sh" "${PROJECT_NAME}.exe" "${PROJECT_SOURCE_DIR}/lib"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                  DEPENDS ${PROJECT_NAME}
                  )

add_custom_target(strip_exe
                  COMMAND ${MSYS2_HOME}/usr/bin/bash.exe "${PROJECT_SOURCE_DIR}/strip_exe.sh" "${PROJECT_NAME}.exe"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                  DEPENDS ${PROJECT_NAME}
                  )

message(STATUS "CMake Config: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: " ${CMAKE_C_COMPILER})
message(STATUS "C Flags: ${CMAKE_C_FLAGS} ")
message(STATUS "Source Directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "External Object: ${APPRES_OBJS}")
message(STATUS "Install destination: " ${INSTALL_DEST})
