echo off
set CONFIGURATION=%1

call copy_drvsyn_binary.bat %CONFIGURATION% eqCore.dll
call copy_drvsyn_binary.bat %CONFIGURATION% eqMatSystem.dll
call copy_drvsyn_binary.bat %CONFIGURATION% eqBaseShaders.dll
call copy_drvsyn_binary.bat %CONFIGURATION% eqD3D9RHI.dll
call copy_drvsyn_binary.bat %CONFIGURATION% eqGLRHI.dll
call copy_drvsyn_binary.bat %CONFIGURATION% eqNullRHI.dll