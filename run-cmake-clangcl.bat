@echo off
setlocal

if not exist "build-clangcl" mkdir "build-clangcl"
pushd build-clangcl

cmake -G "Visual Studio 16 2019" -A x64 -T "clangcl" .. 

popd
endlocal
pause
