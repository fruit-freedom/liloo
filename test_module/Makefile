.PHONY: build
build:
	cmake --build build
	cp build/libtest_module.so .
	stubgen --module libtest_module --output .

.PHONY: build-init
build-init:
	cmake -B build

.PHONY: build-pyi
build-pyi:
	stubgen --module libtest_module --output .

