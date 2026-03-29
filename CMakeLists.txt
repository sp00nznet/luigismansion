cmake_minimum_required(VERSION 3.20)
project(LuigisMansionRecomp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

# Build options
option(LM_BUILD_RECOMPILER "Build the static recompiler tool" ON)
option(LM_BUILD_RUNTIME "Build the game runtime" ON)
option(LM_BUILD_TOOLS "Build auxiliary tools (DOL parser, asset extractor)" ON)

# ===========================================================================
# gcrecomp — Generic GameCube recompilation toolkit (submodule)
# Provides: gcrecomp_gx, gcrecomp_audio, gcrecomp_input, gcrecomp_runtime
# ===========================================================================
add_subdirectory(lib/gcrecomp)

# ===========================================================================
# Tool: DOL/REL Parser & Static Recompiler
# ===========================================================================
# The recompiler is now provided by gcrecomp (gcrecomp_recompiler).
# Use: build/lib/gcrecomp/Release/gcrecomp_recompiler.exe main.dol --output recompiled/
if(LM_BUILD_RECOMPILER AND TARGET gcrecomp_recompiler)
    # gcrecomp_recompiler is built as part of the gcrecomp subdirectory
endif()

# ===========================================================================
# Runtime Library: Luigi's Mansion-specific OS/runtime layer
#
# Game-specific OS HLE, memory map, and function table. Links against
# gcrecomp for GX graphics, audio, and input.
# ===========================================================================
if(LM_BUILD_RUNTIME)
    add_library(lm_runtime STATIC
        src/runtime/ppc_context.cpp
        src/runtime/memory.cpp
        src/runtime/func_table.cpp
        src/runtime/os_funcs.cpp
    )
    target_include_directories(lm_runtime PUBLIC include)
    target_link_libraries(lm_runtime PUBLIC gcrecomp_runtime)

    # ===========================================================================
    # Recompiled Game Code - Auto-generated PPC->C translation
    #
    # These are the recompiled Luigi's Mansion functions split across
    # multiple translation units for parallel compilation.
    # ===========================================================================
    option(LM_BUILD_RECOMPILED "Build the recompiled game code" ON)
    if(LM_BUILD_RECOMPILED AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/recompiled/recomp_funcs.h")
        file(GLOB RECOMP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/recompiled/recomp_*.cpp")
        add_library(lm_recompiled STATIC ${RECOMP_SOURCES})
        target_include_directories(lm_recompiled PRIVATE
            include
            "${CMAKE_CURRENT_SOURCE_DIR}/recompiled"
        )
        target_link_libraries(lm_recompiled PRIVATE lm_runtime)
        # Large translation units need big object file support on MSVC
        if(MSVC)
            target_compile_options(lm_recompiled PRIVATE /bigobj)
        endif()
    endif()

    # Main executable (links recompiled code + runtime + gcrecomp libraries)
    add_executable(lm_launcher
        src/main.cpp
    )
    target_link_libraries(lm_launcher PRIVATE
        lm_runtime
        gcrecomp_tools
        gcrecomp_gx
        gcrecomp_audio
        gcrecomp_input
    )
    if(TARGET lm_recompiled)
        target_link_libraries(lm_launcher PRIVATE lm_recompiled)
        target_compile_definitions(lm_launcher PRIVATE LM_HAS_RECOMPILED)
    endif()
    target_include_directories(lm_launcher PRIVATE include)
    if(WIN32)
        target_link_libraries(lm_launcher PRIVATE d3d11 d3dcompiler dxgi)
    endif()
endif()

# ===========================================================================
# Auxiliary Tools
# ===========================================================================
if(LM_BUILD_TOOLS)
    add_executable(lm_dol_info
        tools/dol_info/main.cpp
    )
    target_link_libraries(lm_dol_info PRIVATE gcrecomp_tools)
    target_include_directories(lm_dol_info PRIVATE include)
endif()
