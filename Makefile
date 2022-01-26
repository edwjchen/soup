all: pass llvm run_pass

pass:
	mkdir -p build && \
	cd build && \
	cmake .. && \
	make 

llvm:
	clang -O0 -emit-llvm -c loop.c

run_pass:
	opt -load ./build/InjectionPass/libInjectionPass.so -inject_function_calls loop.bc --injection main > instrumented_loop.bc
	llvm-dis instrumented_loop.bc

clean:
	rm -rf build 
	rm -rf *.o 
	rm -rf *.ll 