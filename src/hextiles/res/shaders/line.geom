// uniform vec4 _viewport;
// uniform vec2 _line_width;

// Must define one of these to 1 and others to 0
//#define ERHE_LINE_SHADER_SHOW_DEBUG_LINES
//#define ERHE_LINE_SHADER_PASSTHROUGH_BASIC_LINES
//#define ERHE_LINE_SHADER_STRIP

layout(lines) in;

#if ERHE_LINE_SHADER_SHOW_DEBUG_LINES || ERHE_LINE_SHADER_PASSTHROUGH_BASIC_LINES
layout(line_strip, max_vertices = 10) out;
#else
layout(triangle_strip, max_vertices = 6) out;
#endif

in vec3  vs_position[];
in vec4  vs_color[];
in float vs_line_width[];

out vec3  v_position;
out vec2  v_start;
out vec2  v_line;
out vec4  v_color;
out float v_l2;
out float v_line_width;

void main(void)
{
    //  a - - - - - - - - - - - - - - - - b
    //  |      |                   |      |
    //  |      |                   |      |
    //  |      |                   |      |
    //  | - - -start - - - - - - end- - - |
    //  |      |                   |      |
    //  |      |                   |      |
    //  |      |                   |      |
    //  d - - - - - - - - - - - - - - - - c

    vec4 start = gl_in[0].gl_Position;
    vec4 end   = gl_in[1].gl_Position;

#if ERHE_LINE_SHADER_PASSTHROUGH_BASIC_LINES
    gl_Position = start; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = end;   v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    EndPrimitive();
    return;
#endif

    // It is necessary to manually clip the line before homogenization.
    // For reference: OpenGL specification 13.7.
    // Using clip control depth mode zero to one (not default in OpenGL)
    // View volume in depth is 0 < z < w.
    // Clipping against near plane when z < 0
    // Clipping against far  plane when z > w
    //{  Mear plane clipping
    //{
    //    float t0 = start.z + start.w;
    //    float t1 = end.z   + end.w;
    //    if (start.z < 0.0)
    //    {
    //        if (end.z < 0.0)
    //        {
    //            return;
    //        }
    //        start = mix(start, end, (0 - t0) / (t1 - t0));
    //    }
    //    if (end.z < 0.0)
    //    {
    //        end = mix(start, end, (0 - t0) / (t1 - t0));
    //    }
    //}

    // Assume reverse Z - we only need to clip against 'far' = near plane
    {
        float t0 = start.z - start.w;
        float t1 = end.z   - end.w;
        if (t0 > 0) // start.z > start.w
        {
            if (t1 > 0) // end.w > end.w
            {
                return;
            }
            //  mix(x, y, a) = (1 - a)x + ay
            // t0 = Az - Aw; t1 = Bz - Bw; t = t0 / (t0 - t1)
            start = mix(start, end, t0 / (t0 - t1));
        }
        if (t1 > 0.0)
        {
            end = mix(start, end, t0 / (t0 - t1));
        }
    }


    vec2 vpSize         = view.viewport.zw;

    // Compute line axis and side vector in screen space
    vec2 startInNDC     = start.xy / start.w;       //  clip to NDC: homogenize and drop z
    vec2 endInNDC       = end.xy   / end.w;
    vec2 lineInNDC      = endInNDC - startInNDC;
    vec2 startInScreen  = (0.5 * startInNDC + vec2(0.5)) * vpSize + view.viewport.xy;
    vec2 endInScreen    = (0.5 * endInNDC   + vec2(0.5)) * vpSize + view.viewport.xy;
    vec2 lineInScreen   = lineInNDC * vpSize;       //  NDC to window (direction vector)
    vec2 axisInScreen   = normalize(lineInScreen);
    vec2 sideInScreen   = vec2(-axisInScreen.y, axisInScreen.x);    // rotate
    vec2 axisInNDC      = axisInScreen / vpSize;                    // screen to NDC
    vec2 sideInNDC      = sideInScreen / vpSize;
    vec4 axis_start     = vec4(axisInNDC, 0.0, 0.0) * vs_line_width[0];  // NDC to clip (delta vector)
    vec4 axis_end       = vec4(axisInNDC, 0.0, 0.0) * vs_line_width[1];  // NDC to clip (delta vector)
    vec4 side_start     = vec4(sideInNDC, 0.0, 0.0) * vs_line_width[0];
    vec4 side_end       = vec4(sideInNDC, 0.0, 0.0) * vs_line_width[1];

    vec4 a = (start + (side_start - axis_start) * start.w);
    vec4 b = (end   + (side_end   + axis_end  ) * end.w);
    vec4 c = (end   - (side_end   - axis_end  ) * end.w);
    vec4 d = (start - (side_start + axis_start) * start.w);

    v_start = startInScreen;
    v_line  = endInScreen - startInScreen;
    v_l2    = dot(v_line, v_line);

#if ERHE_LINE_SHADER_SHOW_DEBUG_LINES
    gl_Position = gl_in[0].gl_Position; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = gl_in[1].gl_Position; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    EndPrimitive();

    gl_Position = a; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = b; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    EndPrimitive();

    gl_Position = b; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    gl_Position = c; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    EndPrimitive();

    gl_Position = c; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    gl_Position = d; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    EndPrimitive();

    gl_Position = d; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = a; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    EndPrimitive();

#else

#if ERHE_LINE_SHADER_STRIP
#if 1
    gl_Position = d; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = a; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = c; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    gl_Position = b; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
#else
    gl_Position = a; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = d; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = b; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    gl_Position = c; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
#endif
    EndPrimitive();
#else
    gl_Position = d; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = a; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = c; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    EndPrimitive();
    gl_Position = c; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    gl_Position = a; v_position = vs_position[0]; v_color = vs_color[0]; v_line_width = vs_line_width[0]; EmitVertex();
    gl_Position = b; v_position = vs_position[1]; v_color = vs_color[1]; v_line_width = vs_line_width[1]; EmitVertex();
    EndPrimitive();
#endif

#endif
}
