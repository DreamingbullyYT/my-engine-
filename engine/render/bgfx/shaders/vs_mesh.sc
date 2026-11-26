$input a_position, a_normal, a_texcoord0
$output v_texcoord0, v_normal

#include <bgfx_shader.sh>

uniform mat4 u_modelMatrix;

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0));
    v_texcoord0 = a_texcoord0;
    v_normal = normalize(mul(u_modelMatrix, vec4(a_normal.xyz, 0.0)).xyz);
}
