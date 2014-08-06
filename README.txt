# Introduction

sbml2matlab is an executable for translating SBML files to their MATLAB model equivalents.

# Download Installer
[Windows Installer](https://github.com/stanley-gu/sbml2matlab/blob/master/installer/sbml2matlab-latest-win32.exe?raw=true)
* **NOTE:** Adding sbml2matlab to PATH does not work yet. Also, it is recommended that you install in a user home directory and not `Program Files` for testing purposes to avoid needing admin access. 

# Usage
sbml2matlab may be called in four different ways:

### `sbml2matlab.exe`
   * SBML input is from stdin, MATLAB output is to stdout

### `sbml2matlab.exe -input inputFile.sbml`
   * SBML input is from inputFile.sbml, Matlab output is to stdout

### `sbml2matlab.exe -output outputFile.m`
   * SBML input is from stdin, MATLAB output is to outputFile.m

### `sbml2matlab.exe -input inputFile.sbml -output outputFile.m`
   * SBML input is from inputFile.sbml, MATLAB is to outputFile.m

Replace the square brackets with the paths of the input and output files, respectively.

## Example
### `sbml2matlab.exe -output translated.m < mymodel.sbml`
This will pipe in `mymodel.sbml` as the input to `sbml2matlab` and writes the translated MATLAB file to `translated.m` 

# Version Numbering
sbml2matlab versions use the following numbering scheme: **MAJOR.MINOR.PATCH**

# Building From Source Using CMake
## 1. libSBML

* sbml2matlab and NOM depend on libSBML. Follow the [libSBML instructions](http://sbml.org/Software/libSBML/docs/cpp-api/libsbml-installation.html) to download and install libSBML.

## 2. Building sbml2matlab

* Using the CMake GUI, specify location of the sbml2matlab folder, containing the top level CMakeLists.txt file and source code. Specify where you would like the project files to be built.
* Click the Configure button and select the compiler environment of your choice. Options to be set for your build will be highlighted in red.
![config](http://sbml2matlab.googlecode.com/svn/wiki/images/cmake-config.png)
* The following options must be set: 
1. Specify install location of sbml2matlab. For example, the executable and all runtime libraries will be installed in this location.
2. Specify the bin (runtime libraries), include (header files), and lib (import libraries) folders within the libSBML install location. 
![configed](http://sbml2matlab.googlecode.com/svn/wiki/images/cmake-configed.png)
* Click Configure once again so that the red highlighting disappears. Finally, click the Generate button to produce the project files in the build directory.

### Building only NOM
To build NOM without sbml2matlab, specify the source directory in the CMake GUI as the NOM subdirectory. Proceed through the previous steps as in the sbml2matlab build.

## Building in Microsoft Visual Studio
CMake will generate seven Projects within MSVS for sbml2matlab. The below is a description of what building each of these projects does.


1. ALL_BUILD: Compiles all source files. This is the default Build Solution action.
* INSTALL: Compiles all source files and places the output in the location specified in CMAKE_INSTALL_PREFIX, along with the dependencies (libsbml.dll et al.) needed to run it, help files, license, and readme files.
* libsbml2matlab: Compiles sbml2matlab as a shared library.
* NOM: Compiles the NOM as a shared library.
* PACKAGE: Builds the standalone installer.
* sbml2matlab: Compiles sbml2matlab as an executable.
* ZERO_CHECK: Used by CMake to check that project files are up to date relative to the CMakeLists.txt files.

## Building in Unix  
* In the terminal, run `make` in the Build folder, specified in CMake within the "Where to build the binaries" field.
* After `make` is complete, enter in `make install` to install all the program files into the location specified by CMAKE_INSTALL_PREFIX.

# Notes on Dependencies #
## Compile Time
NOM requires libSBML header files and the libSBML import library to compile.
## Runtime
NOM.dll requires libSBML.dll and the shared libraries (DLLs) found in  the libSBML dependencies. All these DLLs are placed in the same install folder as sbml2matlab or any other program that uses NOM.


# Python Bindings

## Building Mac Distribution

The python binding distribution for Mac was produced with the following script:

```
mkdir -p ~/projects/libsbml/build_experimental
cd ~/projects/libsbml && svn co https://svn.code.sf.net/p/sbml/code/branches/libsbml-experimental@20107
cd ~/projects/libsbml/build_experimental && cmake -DCMAKE_INSTALL_PREFIX=/usr/local/libsbml -DENABLE_LAYOUT=OFF -DENABLE_RENDER=OFF -DWITH_PYTHON=ON -DWITH_BZIP2=OFF -DWITH_ZLIB=OFF -DPYTHON_INCLUDE_DIR=/usr/local/include/python2.6 -DPYTHON_LIBRARY=/usr/local/lib/libpython2.6.dylib -DCMAKE_CXX_FLAGS='-stdlib=libstdc++ -mmacosx-version-min=10.6' ../libsbml-experimental
cd ~/projects/libsbml/build_experimental && make -j4 && make install
echo "/usr/local/libsbml/lib/python2.6/site-packages/libsbml" | tee /usr/local/lib/python2.6/site-packages/libsbml.pth

cd ~/projects && git clone https://github.com/stanley-gu/sbml2matlab.git
mkdir -p ~/projects/sbml2matlab/build
cd ~/projects/sbml2matlab/build && cmake .. -DLIBSBML_INCLUDE_DIR=/usr/local/libsbml/include -DCMAKE_INSTALL_PREFIX=/usr/local/sbml2matlab -DWITH_LIBSBML_LIBXML=ON -DLIBSBML_LIBRARY=/usr/local/libsbml/lib/libsbml-static.a -DWITH_PYTHON=ON -DPYTHON_INCLUDE_DIR=/usr/local/include/python2.6 -DPYTHON_LIBRARY=/usr/local/lib/libpython2.6.dylib -DCMAKE_CXX_FLAGS='-fPIC -stdlib=libstdc++ -mmacosx-version-min=10.6'
cd ~/projects/sbml2matlab/build && make -j4 && make install
```
