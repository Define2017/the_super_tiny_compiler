
debug:
	gcc -Wall superTinyCompiler_c.c -o ccg.exe -g
gcc:
	gcc -Wall superTinyCompiler_c.c -o ccg.exe
tcc:
	tcc -Wall superTinyCompiler_c.c -o cct.exe
zcc:
	zig cc -Wall superTinyCompiler_c.c -o ccz.exe
rust:
	rustc superTinyCompiler_r.rs 
#-o rc.exe
zig:
	zig build-exe superTinyCompiler_z.zig 
# -O ReleaseSmall
	
run:
	./*.exe
	
clean:
	rm *.exe