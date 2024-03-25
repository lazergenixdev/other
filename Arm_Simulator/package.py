def package(ctx):
	ctx.file("bin/Debug-windows-x86_64/arm_simulator.exe")
	ctx.file("bin/Debug-windows-x86_64/Fission.dll")
	ctx.file("sandbox/test1.s")
	ctx.file("sandbox/test2.s")
	ctx.file("sandbox/main.s")