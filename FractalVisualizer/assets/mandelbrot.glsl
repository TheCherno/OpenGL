#version 400 core

layout (location = 0) out vec4 o_Color;

layout (location = 1) out uvec4 o_Data;
layout (location = 2) out uvec2 o_Iter;

uniform usampler2D i_Data;
uniform usampler2D i_Iter;

uniform uvec2 size;
uniform dvec2 xRange;
uniform dvec2 yRange;
uniform uint itersPerFrame;
uniform uint frame;

#color

double map(double value, double inputMin, double inputMax, double outputMin, double outputMax)
{
    return outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (value - inputMin);
}

double rand(float s) 
{
    return fract(sin(s * 12.9898) * 43758.5453);
}

void mandelbrot()
{
    uvec2 iter_data = texture(i_Iter, gl_FragCoord.xy / size).xy;
    uint iter = iter_data.x;
    uint total_iters = iter_data.y;

    dvec2 screen_pos = gl_FragCoord.xy;
    dvec2 pos = screen_pos + dvec2(rand(iter), rand(iter + 1));

    dvec2 c;
    c.x = map(pos.x, 0, size.x, xRange.x, xRange.y);
    c.y = map(pos.y, 0, size.y, yRange.x, yRange.y);

    dvec2 z;
    if (frame == 0)
    {
        z = dvec2(0, 0);
        iter = 0;
        total_iters = 0;
    }
    else
    {
        uvec4 data = texture(i_Data, gl_FragCoord.xy / size);
        z.x = packDouble2x32(data.xy);
        z.y = packDouble2x32(data.zw);
    }
    
    int i;
    for (i = 0; i < itersPerFrame && z.x*z.x + z.y*z.y <= 4; i++)
    {
        z = dvec2(z.x*z.x - z.y*z.y, 2.0 * z.x*z.y) + c;
    }

    if (i == itersPerFrame)
    {
        o_Data = uvec4(unpackDouble2x32(z.x), unpackDouble2x32(z.y));
        o_Iter = uvec2(iter, total_iters + i);
        o_Color = vec4(0.0, 0.0, 0.0, 0.0);
    }
    else
    {
        o_Data = uvec4(unpackDouble2x32(0), unpackDouble2x32(0));
        o_Iter = uvec2(iter + 1, 0);
        o_Color = vec4(get_color(int(total_iters) + i).xyz, 1.0 / float(iter + 1));
    }
}

void main()
{
    mandelbrot();
}