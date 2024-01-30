# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

import contextlib
import shutil
import subprocess
from pathlib import Path
import sys
import tempfile

__version__ = "0.0.1"

ext_modules = [
    Pybind11Extension(
        "liloo.libliloo",
        ["liloo/core.cpp"],
    ),
]

DIR = Path(__file__).parent.absolute()

@contextlib.contextmanager
def remove_output(*sources: str):
    try:
        yield
    finally:
        for src in sources:
            shutil.rmtree(src)


with remove_output("liloo/include", "liloo/lib"):
    with tempfile.TemporaryDirectory() as tmpdir:
        subprocess.run(["cmake", "-S", ".", "-B", tmpdir], check=True, cwd=DIR, stdout=sys.stdout, stderr=sys.stderr)
        subprocess.run(["cmake", "--build", tmpdir], check=True, cwd=DIR, stdout=sys.stdout, stderr=sys.stderr)
        subprocess.run(["cmake", "--install", tmpdir, "--prefix", "liloo"], check=True, cwd=DIR, stdout=sys.stdout, stderr=sys.stderr)

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
        packages=[
            'liloo',
            'liloo.include',
            'liloo.lib'
        ],
        package_data={
            'liloo.include': ['liloo/core.h'],
            'liloo.lib': ['*', 'libliloo/*'],
        },
        include_dirs=['include']
    )
