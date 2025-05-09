set PREMAKE_FILE=%cd%\GPUDriven\premake5.lua

cd ..
utils\premake5 --file=%PREMAKE_FILE% vs2022
pause
