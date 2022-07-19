import subprocess

from Cython.Build import cythonize
from setuptools import Extension, find_packages, setup

from jail import __version__

subprocess.call("make")

extensions = [
    Extension(
        "jail/core/core", ["jail/core/core.c"], extra_compile_args=["-std=c99", "-g"]
    )
]

#use setup to creat /usr/bin/pjail for user (-v for debug): pjail -c config.toml -v
setup(
    name="pjail",
    packages=find_packages("./"),
    include_package_data=True,
    install_requires=["cython", "psutil", "toml", "loguru"],
    entry_points={"console_scripts": {"pjail = jail.cmd.jail_cmd:main"}},
    version=__version__,
    ext_modules=cythonize(extensions, language=3),
)
