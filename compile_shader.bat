@echo on
set input_file = %~dp0Raytracing

%~dp0ThirdParty/dxc/dxc.exe /Zi /Od /Fo %~dp0Resource/Shader/Raytracing.cso /T lib_6_5 /nologo %~dp0Source/Shader/Raytracing.hlsl

pause