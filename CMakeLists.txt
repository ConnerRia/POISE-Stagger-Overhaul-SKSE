cmake_minimum_required(VERSION 3.18)


project(loki_POISE VERSION 0.3.0.0 LANGUAGES CXX)
set(PROJECT_PLUGIN_NAME "POISEBREAKER NG")

# ---- Options ----
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(GNUInstallDirs)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
find_package(tomlplusplus CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Do not delete the below begin and end statements.
# They are used by the globber py script for replacement
# run python scripts/glob_src.py, no need to manually fill source files.
### BEGIN SOURCE FILES ###
set(SOURCES
	src/ActorCache.cpp
	src/Loki_PluginTools.cpp
	src/main.cpp
	src/PCH.cpp
	src/POISE/PoiseMod.cpp
	src/POISE/TrueHUDControl.cpp
)

set(HEADERS
	src/ActorCache.h
	src/Loki_PluginTools.h
	src/PCH.h
	src/POISE/PoiseMod.h
	src/POISE/TrueHUDAPI.h
	src/POISE/TrueHUDControl.h
)
### END SOURCE FILES ###

add_commonlibsse_plugin(${PROJECT_NAME}
    # The plugin's name, defaults to target.
    NAME "${PROJECT_PLUGIN_NAME}"

    # The plugin's author, empty by default.
    AUTHOR "KAOS (a.k.a. xILARTH)"

    # The support email address, empty by default.
    EMAIL "kaos4d1@gmail.com"

    # Indicates the plugin is compatible with all runtimes via address library. This is the default if no
    # other compatibilility mode is specified. Can be used with USE_SIGNATURE_SCANNING but not
    # COMPATIBLE_RUNTIMES.
    USE_ADDRESS_LIBRARY

    # Indicates the plugin is compatible with all runtimes via signature scanning.  Can be used with
    # USE_ADDRESS_LIBRARY but not COMPATIBLE_RUNTIMES.
    USE_SIGNATURE_SCANNING

    # List of the sources to include in the target, as would be the parameters to add_library.
    SOURCES ${SOURCES} ${HEADERS} ${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE spdlog::spdlog
    PRIVATE tomlplusplus::tomlplusplus
    PRIVATE fmt::fmt
)

target_compile_features(
    ${PROJECT_NAME}
    PUBLIC
        cxx_std_23
)

target_precompile_headers(
    ${PROJECT_NAME}
    PRIVATE
        src/PCH.h
)

target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC
        WINVER=0x0601	# windows 7, minimum supported version by skyrim special edition
        _WIN32_WINNT=0x0601
        "$<$<BOOL:${SKSE_SUPPORT_XBYAK}>:SKSE_SUPPORT_XBYAK=1>"
        "$<$<BOOL:${ENABLE_SKYRIM_SE}>:ENABLE_SKYRIM_SE=1>"
        "$<$<BOOL:${ENABLE_SKYRIM_AE}>:ENABLE_SKYRIM_AE=1>"
        "$<$<BOOL:${ENABLE_SKYRIM_VR}>:ENABLE_SKYRIM_VR=0>"
        "$<$<BOOL:${WAIT_FOR_DEBUGGER}>:WAIT_FOR_DEBUGGER=1>"
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "$ENV{MO_SSE_INSTANCE}/mods/${PROJECT_PLUGIN_NAME}/SKSE/Plugins/"
)

find_package(Python)
install(CODE "execute_process(COMMAND \"${_Python_EXECUTABLE}\" \"${CMAKE_CURRENT_SOURCE_DIR}/scripts/mkrelease.py\" -v ${PROJECT_VERSION} -n \"${PROJECT_PLUGIN_NAME}\" -f \"$<TARGET_FILE:${PROJECT_NAME}>\" -o \"${PROJECT_NAME}\")")
