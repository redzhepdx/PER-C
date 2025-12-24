from setuptools import setup, Extension

ext = Extension(
    "per",
    sources=["per.c"],
    extra_compile_args=["-O3"],
)

setup(
    name="per",
    version="0.1.0",
    ext_modules=[ext],
)