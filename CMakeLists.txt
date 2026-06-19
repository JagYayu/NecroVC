cmake_minimum_required(VERSION 3.28)

project(NecroVC
    VERSION 0.1.0.0 # 同步更改 version.rc
    LANGUAGES C CXX
    DESCRIPTION "Voice Chat Support for Crypt of the Necrodancer: Synchrony"
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

foreach(CONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${PROJECT_SOURCE_DIR}/Bin/${CONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${PROJECT_SOURCE_DIR}/Bin/${CONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} "${PROJECT_SOURCE_DIR}/Bin/${CONFIG}/Lib")
endforeach()

# 定位到 lua51.dll 动态库所在目录
set(LUA51_DLL_DIR "D:/Program Files (x86)/Steam/steamapps/common/Crypt of the NecroDancer/Necrodancer64/")
set(LUA51_DLL "${LUA51_DLL_DIR}/lua51.dll")
set(LUA51_DEF_GEN "${LUA51_DLL_DIR}/lua51.def")
set(LUA51_LIB_GEN "${CMAKE_BINARY_DIR}/lua51.lib")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIB_MACHINE "x64")
else()
    set(LIB_MACHINE "x86")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty/opus" EXCLUDE_FROM_ALL)

add_custom_command(
    OUTPUT ${LUA51_LIB_GEN}
    COMMAND dumpbin /EXPORTS "${LUA51_DLL}" > "${CMAKE_BINARY_DIR}/exports.txt"
    COMMAND ${CMAKE_COMMAND} -E echo "LIBRARY lua51.dll" > "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "EXPORTS" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_pushinteger" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_setfield" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "luaL_checkinteger" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_getfield" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_settop" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_type" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_pushboolean" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_pushfstring" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_pushstring" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_pushnil" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_newtable" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_rawseti" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "luaL_register" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "luaL_optinteger" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_pushlstring" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "lua_createtable" >> "${LUA51_DEF_GEN}"
    COMMAND ${CMAKE_COMMAND} -E echo "luaL_checklstring" >> "${LUA51_DEF_GEN}"
    # 追加用到的函数 TODO 移除不用的
    COMMAND lib /def:"${LUA51_DEF_GEN}" /out:"${LUA51_LIB_GEN}" /machine:${LIB_MACHINE}
    COMMENT "Generating import library from ${LUA51_DLL}"
)

add_library(lua51 SHARED IMPORTED)
set_target_properties(lua51 PROPERTIES
    IMPORTED_LOCATION "${LUA51_DLL}"
    IMPORTED_IMPLIB "${LUA51_LIB_GEN}"
)

add_custom_target(generate_lua51_lib DEPENDS ${LUA51_LIB_GEN})
add_dependencies(lua51 generate_lua51_lib)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_library(NecroVC SHARED ${SRC_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/Misc/win_version_x64.rc")
    else()
        add_library(NecroVC SHARED ${SRC_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/Misc/win_version_x32.rc")
    endif()
else()
    add_library(NecroVC SHARED ${SRC_FILES})
endif()

target_compile_definitions(NecroVC PRIVATE
    NECROVC_EXPORTS
    MINIAUDIO_IMPLEMENTATION
)

target_link_libraries(NecroVC
    opus
    lua51
)

set_target_properties(NecroVC PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(MSVC)
    set_target_properties(NecroVC PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
endif()

target_include_directories(NecroVC PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty/opus/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty/miniaudio"
    "${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty/lua5.1.4"
)
