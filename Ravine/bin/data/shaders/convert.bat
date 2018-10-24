for /r %%a in (*.vert) do (
  .\..\compilers\glslangValidator.exe -V %%~nxa -o %%~nxa.spv
)
for /r %%a in (*.frag) do (
  .\..\compilers\glslangValidator.exe -V %%~nxa -o %%~nxa.spv
)
pause