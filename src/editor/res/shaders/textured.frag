in      vec2  v_texcoord;
in flat uvec2 v_texture;

float srgb_to_linear(float x)
{
    if (x <= 0.04045)
    {
        return x / 12.92;
    }
    else
    {
        return pow((x + 0.055) / 1.055, 2.4);
    }
}

vec4 srgb_to_linear(vec4 v)
{
    return vec4(
        srgb_to_linear(v.r),
        srgb_to_linear(v.g),
        srgb_to_linear(v.b),
        v.a
    );
}

void main()
{
#if defined(ERHE_BINDLESS_TEXTURE)
    sampler2D s_texture = sampler2D(v_texture);
    out_color = texture(s_texture, v_texcoord);
#else
    out_color = texture(s_texture[v_texture.x], v_texcoord);
#endif
    out_color = vec4(1.0, 0.0, 1.0, 1.0);
}
