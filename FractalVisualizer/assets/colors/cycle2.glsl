#uniform colorMult 250 1 NULL;
vec3 colors[] = vec3[](
	vec3(0.0, 0.0, 0.0),
	vec3(0.6, 0.22, 0.26),
	vec3(1.0, 0.86, 0.56),
	vec3(0.13, 0.31, 0.5),
	vec3(0.0, 0.0, 0.0)
);
vec3 get_color(int iters)
{
	float x = iters / colorMult;
    x = mod(x, colors.length() - 1);

    if (x == colors.length())
        x = 0.0;

    if (floor(x) == x)
        return colors[int(x)];

    int i = int(floor(x));
    float t = mod(x, 1);
    return mix(colors[i], colors[i+1], t);
}