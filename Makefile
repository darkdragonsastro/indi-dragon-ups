.PHONY: clean

clean:
	@rm -Rf build

build: clean
	@mkdir build && cd build && cmake .. && cmake --build . && cpack .
