# reversi

Simple reversi game homework.

## How to build

1. The include headers of the external libraries are already included in the `include` directory.
   However, the static library of Nana (libnana.a) needs to be manually built and put into
   the `lib/` directory.
2. Configure using CMake. The project has no special configuration variables.
3. If all goes well, you should have a running executable in your build directory!

## Third party libraries

+ doctest: see `include/doctest.h` for license and copyright notice
+ nana: see `include/nana/gui.hpp` for license and copyright notice
+ nlohmann/json: see `include/nlohmann/json.hpp` for license and copyright notice
+ Icon from [Chessboard icons created by Muhamad Ulum - Flaticon](https://www.flaticon.com/free-icons/chessboard)
