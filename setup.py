# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages

__version__ = "0.0.1"

ext_modules = [
    Pybind11Extension(
        "liloo.libliloo",
        ["liloo/core.cpp"],
    ),
]

setup(
    name="liloo",
    version=__version__,
    author="Alexandr Kozlovsky",
    description="Asynchronous Python connect for C++ modules",
    long_description="",
    ext_modules=ext_modules,
    cmdclass={
        "build_ext": build_ext,
    },
    zip_safe=False,
    python_requires=">=3.8",
    packages=find_packages(),
)
