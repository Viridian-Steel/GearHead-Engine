@echo off
cd %1
mkdir Build
cd Build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Release
cmake --build . --config Debug
cmake --install . --config Release --prefix %2
cmake --install . --config Debug --prefix %2
