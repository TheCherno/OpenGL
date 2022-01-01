#uniform colorMult 200 1 NULL;
vec3 get_color(int i)
{
    float x = i / colorMult;
    float n1 = sin(x) * 0.5 + 0.5;
    float n2 = cos(x) * 0.5 + 0.5;
    return vec3(n1, n2, 1.0) * 1;
}