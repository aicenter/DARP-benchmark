import setuptools
from setuptools import setup

setup(
	name='darpbenchmark',
	version='0.0.1',
	description='DAR benchmark tools',
	author='David Fiedler',
	author_email='david.fido.fiedler@gmail.com',
	license='MIT',
	packages=setuptools.find_packages(),
	install_requires=[
		'numpy',
		'pandas',
		'matplotlib',
		'tqdm',
		'typing',
		'pyyaml',
		'scipy',
		'h5py',
		'gurobipy',
		'pytest',
		'darpinstances'
	],
	python_requires='>=3.8'
)
