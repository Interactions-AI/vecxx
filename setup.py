import sys

# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.0.4"

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

ext_modules = [
    Pybind11Extension("vecxx",
        ["src/vecxx.cpp"],
        include_dirs=["include"],
        define_macros = [('VERSION_INFO', __version__),
            ('_CRT_RAND_S', 1),],
        ),
]

setup(
    name="vecxx",
    version=__version__,
    description="Vectorize some text",
    long_description=open('README.md').read(),
    long_description_content_type="text/markdown",
    author="dpressel",
    author_email="dpressel@gmail.com",
    python_requires='>=3.6',
    ext_modules=ext_modules,
    install_requires=["pybind11>=2.5.0"],
    extras_require={
        'test': ['pytest', 'pytest-forked'],
    },
    cmdclass={'build_ext': build_ext},
    package_data={
        'vecxx': [
            'include/vecxx/vecxx.h',
            'include/vecxx/bpe.h',
            'include/vecxx/utils.h'
        ]
    },
    include_package_data=True,
    zip_safe=False,
)
