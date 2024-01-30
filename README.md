Develop installation
--------------------

- Use command from `Makefile` to build and test project.
- `pip install -r requirements.txt`


TODO
----

1. Class that provides interface like `std::function` but can modify captured objects. \
    It is important to move captured objects instead of copying due to performane and \
    avoiding destructors for some types. 
2. Class that provides functionality for deffered `numpy.ndarray` (`pybind11::array_t`) object construction.
3. Proxy exception / errors from cpp resolving `asyncio.Future` with exceptions.
4. Retrieve `liloo` logic in python module, shared library and include files
5. Append generic type hints like `Future<std::string>` for returning type like `Future[str]`. Mb using `pybind11_generic` library.



Generate interface file (`.pyi`)
--------------------------------

```
stubgen --module asyncie --output .
```



Integration
-----------

- Generate CPP module using CMake `add_liloo_module()` function
- Install `liloo` package using `pip`



Architecture
------------

```txt
liloo.py - CompletitionQueue wraped singleton instance + `futurize()` method
libliloo.so - CompletitionQueue implementation (Could be split to CompletitionQueue.so and python module)

test_module.py
libtest_module.so
```


Sources
-------

https://stackoverflow.com/questions/37531846/nm-symbol-output-t-vs-t-in-a-shared-so-library



Develop
-------
Build package
```bash
# Develop
python setup.py build

# Wheel
python setup.py bdist_wheel
```
