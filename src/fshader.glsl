#version 150

in  vec3 fragPos;
in  vec3 fragNormal;
in  vec4 fragColor;

out vec4 fColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform vec3 lightSpecular;

uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);

    vec3 V = normalize(viewPos - fragPos);
    vec3 H = normalize(L + V);
    float spec = 0.0;
    if (diff > 0.0)
        spec = pow(max(dot(N, H), 0.0), materialShininess);

    vec3 ambient  = lightAmbient  * materialAmbient;
    vec3 diffuse  = lightDiffuse  * materialDiffuse  * diff;
    vec3 specular = lightSpecular * materialSpecular * spec;

    vec3 color = (ambient + diffuse + specular) * fragColor.rgb;
    fColor = vec4(color, fragColor.a);
}
