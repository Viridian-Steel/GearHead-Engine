IF %1 == "" ( echo need source ) 
IF %2 == "" ( echo need output destination)
cd %1
mkdir Build
cd Build
cmake ..
cmake --build .
cmake --install . --config Debug --prefix %2

:: 1 = src, 2 = Output dir