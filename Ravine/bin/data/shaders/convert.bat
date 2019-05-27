for /r %%a in (*.vert) do (
  glslangValidator -V %~dp0%%~nxa -o %~dp0%%~nxa.spv
)
for /r %%a in (*.frag) do (
  glslangValidator -V %~dp0%%~nxa -o %~dp0%%~nxa.spv
)
pause