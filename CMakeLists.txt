# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
set(SUBMODULE_DIR ${CMAKE_SOURCE_DIR}/submodules)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("TicTacToe")

find_package(imgui CONFIG REQUIRED)

file(GLOB ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/Engine/source/*.cpp" "${CMAKE_SOURCE_DIR}/Engine/source/Objects/*.cpp")
add_library(EngineLib STATIC ${ENGINE_SOURCES})
target_include_directories(EngineLib PUBLIC ${CMAKE_SOURCE_DIR}/Engine/include ${CMAKE_SOURCE_DIR}/Engine/include/Objects ${SUBMODULE_DIR}/IconFontCppHeaders)
target_link_libraries(EngineLib d2d1.lib dwrite.lib d3d11.lib dxgi.lib imgui::imgui)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET EngineLib PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory ("Game/source")
add_subdirectory ("Editor/source")

file(COPY data DESTINATION "${CMAKE_BINARY_DIR}/bin/Editor")
file(COPY data DESTINATION "${CMAKE_BINARY_DIR}/bin/Game")