# CMakeList.txt : CMake project for TicTakToe, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (TicTakToe WIN32 "main.cpp" "Application.cpp" "stdafx.cpp" "GameEngine.cpp" "Game.cpp" "TextObject.cpp")
target_include_directories(TicTakToe PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(TicTakToe d2d1.lib dwrite.lib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TicTakToe PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
