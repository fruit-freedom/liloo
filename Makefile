.PHONY: build
build:
	cmake --build build
	cp build/libliloo.so liloo
	# cd liloo; stubgen --module libliloo --output .

.PHONY: build-init
build-init:
	cmake -B build

.PHONY: test-build
test-build:
	$(MAKE) -C test_module build

.PHONY: test-build-init
test-build-init:
	$(MAKE) -C test_module build-init


