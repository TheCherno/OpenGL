# OpenGL-Core
Work-in-progress OpenGL library that aims to provide a powerful sandbox for you to learn or experiment with OpenGL, and graphics programming in general.  

These repo add **macOS support (using CMake)**. The original repo is at https://github.com/TheCherno/OpenGL.

## Usage

### Windows

```
git clone --recursive https://github.com/TheCherno/OpenGL
```

Run `scripts/Win-Premake.bat` and open `OpenGL-Sandbox.sln` in Visual Studio 2019. `OpenGL-Sandbox/src/SandboxLayer.cpp` contains the example OpenGL code that's running.


### MacOS

#### Download, Build and Run
```
git clone --recursive https://github.com/TheCherno/OpenGL
cmake .
make

cd OpenGL-Examples
./OpenGL-Examples 
```
