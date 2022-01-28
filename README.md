# Fractal visualizer
WIP fractal visualizer build on top of https://github.com/TheCherno/OpenGL.

## Main functionality

- You can pan and zoom by dragging and using the mouse wheel.
- Right click the mandelbrot image to set the c value for the julia set.
- CTRL + click will set the center of the screen to mouse position.

## Build

Currently only "officially" supports Windows.

```
git clone --recursive https://github.com/NullSeile/FractalVisualizer.git
```

Run `scripts/Win-Premake.bat` and open `FractalVisualizer.sln` in Visual Studio 2022. `FractalVisualizer/src/FractalLayer.cpp` contains the example OpenGL code that's running.
