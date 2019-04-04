@echo off

rem Auxiliary script compiling GLSL shaders into SPIR-V format.
for %%i in (glslangValidator.exe) do ( if "%%~$PATH:i" == "" ( call "%~dp0../../env.bat" ) )
for %%i in (glslangValidator.exe) do (
  if "%%~$PATH:i" == "" (
    echo "Error: could not find %%i"
	exit /B
  )
)

set "aClient=vulkan100"
set "aDebug="
rem set "aDebug=-g"

rem Generate set of icons with standard resolutions.
pushd "%~dp0"
setlocal EnableDelayedExpansion
for /r %%i in (*.vs) do (
  set "aBase=%%~Ni"
  set "aDst=!aBase!_vs_spv.pxx"
  rem echo aBase=!aBase!
  if exist "!aDst!" del "!aDst!"
  glslangValidator.exe --client %aClient% %aDebug% -S vert "%%i" --vn "!aBase!_vs_spv" -o "!aDst!"
)
for /r %%i in (*.fs) do (
  set "aBase=%%~Ni"
  set "aDst=!aBase!_fs_spv.pxx"
  rem echo aBase=!aBase!
  if exist "!aDst!" del "!aDst!"
  glslangValidator.exe --client %aClient% %aDebug% -S frag "%%i" --vn "!aBase!_fs_spv" -o "!aDst!"
)
popd
