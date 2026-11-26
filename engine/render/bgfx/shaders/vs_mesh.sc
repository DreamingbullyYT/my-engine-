$input a_position, a_normal, a_texcoord0
$output v_texcoord0, v_normal

#include <bgfx_shader.sh>

uniform mat4 u_modelMatrix;

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0));
    v_texcoord0 = a_texcoord0;
    mat3 normal_matrix = transpose(inverse(mat3(u_modelMatrix)));
    v_normal = normalize(mul(normal_matrix, a_normal.xyz));
}
