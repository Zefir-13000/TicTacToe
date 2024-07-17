# CMakeList.txt : CMake project for TicTakToe, include source and define
# project specific logic here.
#

#file(GLOB IMGUI_SOURCES "${CMAKE_SOURCE_DIR}/Engine/include/imgui/*.cpp")

# Add source to this project's executable.
add_executable (TicTacToeEditor WIN32 "main.cpp" "GameEditor.cpp" "Application.cpp")
set_target_properties(TicTacToeEditor PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Editor")
target_include_directories(TicTacToeEditor PUBLIC ${CMAKE_SOURCE_DIR}/Editor/include PUBLIC ${CMAKE_SOURCE_DIR}/Engine/include)
target_link_libraries(TicTacToeEditor EngineLib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TicTacToeEditor PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
