:: Batch file!!
echo OFF
fxc PS.hlsl /T"ps_4_0" /Fo"PS.cso" /nologo
fxc VS.hlsl /T"vs_4_0" /Fo"VS.cso" /nologo
fxc ComputeShader.hlsl /T"cs_5_0" /Fo"ComputeShader.cso" /nologo
cl Window.cpp Graphics.cpp WinMain.cpp /O2 /FeApp.exe /nologo