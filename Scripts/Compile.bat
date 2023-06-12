@echo off

IF "%~3" == "" ( GOTO exCmake ) ELSE ( GOTO inCmake )


:exCmake
cd %1
mkdir Build
cd Build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Release
cmake --build . --config Debug
cmake --install . --config Release --prefix %2
cmake --install . --config Debug --prefix %2
GOTO Exit


:inCmake
cd %1
mkdir Build
cd Build
%3 .. -DCMAKE_BUILD_TYPE=Release
%3 .. -DCMAKE_BUILD_TYPE=Debug
%3 --build . --config Release
%3 --build . --config Debug
%3 --install . --config Release --prefix %2
%3 --install . --config Debug --prefix %2
GOTO Exit

:Exit
