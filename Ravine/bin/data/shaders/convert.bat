for /r %%a in (*.vert) do (
  %~dp0..\compilers\glslangValidator.exe -V %~dp0%%~nxa -o %~dp0%%~nxa.spv
)
for /r %%a in (*.frag) do (
  %~dp0..\compilers\glslangValidator.exe -V %~dp0%%~nxa -o %~dp0%%~nxa.spv
)
pause