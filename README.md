# Fractal visualizer
WIP fractal visualizer build on top of https://github.com/TheCherno/OpenGL.

## Main functionality

- You can pan and zoom by dragging and using the mouse wheel.
- Left click the mandelbrot set to set the julia c to the mouse location.
- CTRL + left click to set the center to the mouse location.
- Middle mouse button to show the first iterations of the equation.
- Hold CTRL while releasing the middle mouse button to continuously show the iterations.

## Build

Currently only "officially" supports Windows.

```
git clone --recursive https://github.com/NullSeile/FractalVisualizer.git
```

Run `scripts/Win-Premake.bat` and open `FractalVisualizer.sln` in Visual Studio 2022. `FractalVisualizer/src/FractalLayer.cpp` contains the example OpenGL code that's running.
