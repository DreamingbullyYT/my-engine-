#include <bgfx_shader.sh>

$input v_texcoord0, v_color0

SAMPLER2D(s_texColor, 0);
uniform vec4 u_tint;

void main()
{
    gl_FragColor = texture2D(s_texColor, v_texcoord0) * v_color0 * u_tint;
}
