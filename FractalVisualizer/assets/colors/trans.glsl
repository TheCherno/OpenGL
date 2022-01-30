#uniform colorMult 50 1 3 NULL;
#uniform offset 0 0.02 0 2;
#define PI 3.1415926535
vec3 get_color(int i)
{
    float x = i / colorMult + offset;
    float n1 = sin(PI*x) * 0.5 + 0.5;
    float n2 = cos(PI*x) * 0.5 + 0.5;
    return vec3(n1, n2, 1.0);
}