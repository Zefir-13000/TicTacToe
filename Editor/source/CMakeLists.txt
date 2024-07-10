# CMakeList.txt : CMake project for TicTakToe, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (TicTacToeEditor WIN32 "main.cpp" "GameEditor.cpp")
target_include_directories(TicTacToeEditor PUBLIC ${CMAKE_SOURCE_DIR}/Editor/include)
target_link_libraries(TicTacToeEditor d2d1.lib dwrite.lib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TicTacToeEditor PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
