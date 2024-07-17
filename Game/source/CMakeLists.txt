# CMakeList.txt : CMake project for TicTakToe, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (TicTacToe WIN32 "main.cpp" "Application.cpp" "stdafx.cpp" "Game.cpp")
set_target_properties(TicTacToe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Game")
target_include_directories(TicTacToe PUBLIC ${CMAKE_SOURCE_DIR}/Game/include PUBLIC ${CMAKE_SOURCE_DIR}/Engine/include)
target_link_libraries(TicTacToe d2d1.lib dwrite.lib EngineLib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TicTacToe PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
