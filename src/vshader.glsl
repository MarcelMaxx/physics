#version 150

in  vec4 vPosition;
in  vec4 vNormal;
in  vec4 vColor;

out vec3 fragPos;
out vec3 fragNormal;
out vec4 fragColor;

uniform mat4 mProject;
uniform mat4 mView;
uniform mat4 mModel;

void main()
{
    vec4 worldPos = mModel * vPosition;
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(mModel))) * vNormal.xyz;
    fragColor = vColor;

    gl_Position = mProject * mView * worldPos;
}
