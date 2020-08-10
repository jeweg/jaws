@echo off
setlocal

if not exist "build-msvc" mkdir "build-msvc"
pushd build-msvc

cmake -G "Visual Studio 16 2019" -A x64 .. 

popd
endlocal
pause
