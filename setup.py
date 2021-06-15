import sys
import setuptools
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11


__version__ = "0.0.3"


class About(object):
    NAME="vecxx"
    VERSION=__version__
    AUTHOR="dpressel"
    DESCRIPTION="vectorize some text"


ext_modules = [
    Extension(
        'vecxx',
        ['src/vecxx.cpp'],
        include_dirs=[
            'include/',
            pybind11.get_include(False),
            pybind11.get_include(True)
        ],
        language="c++"
    )
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14/17] compiler flag.
    The newer version is prefered over c++11 (when it is available).
    """
    flags = ['-std=c++17', '-std=c++14', '-std=c++11']

    for flag in flags:
        if has_flag(compiler, flag): return flag

    raise RuntimeError('Unsupported compiler -- at least C++11 support '
                       'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }
    l_opts = {
        'msvc': [],
        'unix': [],
    }

    if sys.platform == 'darwin':
        darwin_opts = ['-stdlib=libc++', '-mmacosx-version-min=10.7']
        c_opts['unix'] += darwin_opts
        l_opts['unix'] += darwin_opts

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        link_opts = self.l_opts.get(ct, [])
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/D_CRT_RAND_S')
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
        build_ext.build_extensions(self)


setup(
    name=About.NAME,
    version=About.VERSION,
    description=About.DESCRIPTION,
    long_description=open('README.md').read(),
    long_description_content_type="text/markdown",
    author=About.AUTHOR,
    python_requires='>=3.6',
    ext_modules=ext_modules,
    install_requires=["pybind11>=2.5.0"],
    extras_require={
        'test': ['pytest', 'pytest-forked'],
    },
    cmdclass={'build_ext': BuildExt},
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
