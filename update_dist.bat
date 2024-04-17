CD dist
RMDIR /S /Q .\resources
XCOPY ..\resources\ .\resources\ /S /E
XCOPY ..\out\build\Release\three_d_renderer.exe .\ /Y
CD ..
