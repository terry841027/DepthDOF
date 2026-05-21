cmake_minimum_required(VERSION 3.15)
project(ZDepthDOF VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Plugin DLL ─────────────────────────────────────────────────────────────
add_library(ZDepthDOF SHARED src/ZDepthDOF.cpp)

target_include_directories(ZDepthDOF PRIVATE src)

# Only needs Windows SDK (comes with VS) — no external dependencies
target_link_libraries(ZDepthDOF PRIVATE opengl32)

set_target_properties(ZDepthDOF PROPERTIES
    OUTPUT_NAME "ZDepthDOF"
    PREFIX      ""
)

# ── Auto-copy to Resolume (local builds only) ──────────────────────────────
set(DEST "$ENV{USERPROFILE}/Documents/Resolume Arena/Extra Effects")
if(EXISTS "${DEST}")
    add_custom_command(TARGET ZDepthDOF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:ZDepthDOF>"
            "${DEST}/ZDepthDOF.dll"
        COMMENT "Deployed → Resolume Arena/Extra Effects/ZDepthDOF.dll"
    )
endif()
