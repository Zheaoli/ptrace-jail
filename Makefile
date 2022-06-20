all: build-cython

.PHONY: build-cython
build-cython:
	cython jail/core/core.pyx