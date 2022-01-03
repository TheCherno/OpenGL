#uniform colorMult 250 1 NULL;
vec3 rgb(int r, int g, int b) { return vec3(r, g, b) / 255.0; }
vec3 colors[] = vec3[](
	rgb(0, 0, 0),
	rgb(154, 57, 65),
	rgb(255, 221, 136),
	rgb(31, 77, 130),
	rgb(0, 0, 0)
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