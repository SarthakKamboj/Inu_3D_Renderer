RMDIR /S /Q .\out\build\Debug\resources\shaders
RMDIR /S /Q .\out\build\Release\resources\shaders
XCOPY .\resources\shaders\ .\out\build\Debug\resources\shaders\ /S /E
XCOPY .\resources\shaders\ .\out\build\Release\resources\shaders\ /S /E
