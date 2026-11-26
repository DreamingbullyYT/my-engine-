$input v_texcoord0, v_normal

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 u_tint;
uniform vec4 u_lightDir;

void main()
{
    vec3 n = normalize(v_normal);
    float diffuse = max(dot(n, normalize(u_lightDir.xyz)), 0.0);
    float lighting = 0.22 + diffuse * 0.78;
    gl_FragColor = texture2D(s_texColor, v_texcoord0) * u_tint * vec4(vec3(lighting), 1.0);
}
