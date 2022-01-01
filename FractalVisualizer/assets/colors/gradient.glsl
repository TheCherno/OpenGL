#uniform colorMult 200 1 NULL;
vec3 get_color(int i)
{
    float t = exp(-i / colorMult);
    return mix(vec3(1, 1, 1), vec3(0.1, 0.1, 1), t);
}