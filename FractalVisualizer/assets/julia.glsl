#version 400 core

layout (location = 0) out vec4 o_Color;

layout (location = 1) out uvec4 o_Data;
layout (location = 2) out uvec2 o_Iter;

uniform usampler2D i_Data;
uniform usampler2D i_Iter;

uniform uvec2 i_Size;
uniform dvec2 i_xRange;
uniform dvec2 i_yRange;
uniform uint i_ItersPerFrame;
uniform uint i_Frame;

uniform dvec2 i_JuliaC;

#color

double map(double value, double inputMin, double inputMax, double outputMin, double outputMax)
{
    return outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (value - inputMin);
}

double rand(float s) 
{
    return fract(sin(s * 12.9898) * 43758.5453);
}

void julia(dvec2 c, uvec2 size, dvec2 xRange, dvec2 yRange)
{
    uvec2 iter_data = texture(i_Iter, gl_FragCoord.xy / size).xy;
    uint iter = iter_data.x;
    uint total_iters = iter_data.y;

    dvec2 screen_pos = gl_FragCoord.xy;
    dvec2 pos = screen_pos + dvec2(rand(iter), rand(iter + 1));

    dvec2 z;
    if (i_Frame == 0)
    {
        z.x = map(pos.x, 0, size.x, xRange.x, xRange.y);
        z.y = map(pos.y, 0, size.y, yRange.x, yRange.y);
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
    for (i = 0; i < i_ItersPerFrame && z.x*z.x + z.y*z.y <= 4; i++)
    {
        z = dvec2(z.x*z.x - z.y*z.y, 2.0 * z.x*z.y) + c;
    }

    if (i == i_ItersPerFrame)
    {
        o_Data = uvec4(unpackDouble2x32(z.x), unpackDouble2x32(z.y));
        o_Iter = uvec2(iter, total_iters + i);
        o_Color = vec4(0.0, 0.0, 0.0, 0.0);
    }
    else
    {
        o_Data = uvec4(
            unpackDouble2x32(map(pos.x, 0, size.x, xRange.x, xRange.y)), 
            unpackDouble2x32(map(pos.y, 0, size.y, yRange.x, yRange.y))
        );
        
        o_Iter = uvec2(iter + 1, 0);
        o_Color = vec4(get_color(int(total_iters) + i).xyz, 1.0 / float(iter + 1));
    }
}

void main()
{
    julia(dvec2(0.28, 0.008), i_Size, i_xRange, i_yRange);
}