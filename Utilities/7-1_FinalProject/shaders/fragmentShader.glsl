#version 330 core
out vec4 fragmentColor;

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
}; 

struct DirectionalLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct PointLight {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    // Distance attenuation terms for each point light.
    float constant;
    float linear;
    float quadratic;

    bool bActive;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       

    bool bActive;
};

#define TOTAL_POINT_LIGHTS 5

uniform bool bUseTexture=false;
uniform bool bUseDetailTexture=false;
uniform bool bUseLighting=false;
uniform vec4 objectColor = vec4(1.0f);
uniform vec3 viewPosition;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[TOTAL_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform sampler2D objectTexture;
uniform sampler2D detailTexture;
uniform vec2 UVscale = vec2(1.0f, 1.0f);
uniform vec2 detailUVScale = vec2(1.0f, 1.0f);
uniform float detailBlendFactor = 0.0f;
uniform float detailBlendGlobal = 1.0f;

// the scaled texture coordinate to use in calculations
vec2 fragmentTextureCoordinateScaled = fragmentTextureCoordinate * UVscale;
vec2 detailTextureCoordinateScaled = fragmentTextureCoordinate * detailUVScale;

// function prototypes
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 surfaceColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 surfaceColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 surfaceColor);
vec4 SampleTextureLinear();
vec4 SampleDetailTextureLinear();
vec4 SampleSurfaceTextureLinear();
vec3 GetObjectColorLinear();
vec3 ApplyGamma(vec3 linearColor);

void main()
{   
    // Do lighting in linear color space, then apply gamma once at the end.
    vec4 outputColorLinear = vec4(0.0f);
    vec4 surfaceTextureLinear = vec4(1.0f);
    vec3 surfaceColorLinear = GetObjectColorLinear();

    if (bUseTexture == true)
    {
        // Base texture + optional detail texture are blended here once.
        surfaceTextureLinear = SampleSurfaceTextureLinear();
        surfaceColorLinear = surfaceTextureLinear.rgb;
    }

    if(bUseLighting == true)
    {
        vec3 phongResult = vec3(0.0f);
        // Properties needed for Phong terms.
        vec3 norm = normalize(fragmentVertexNormal);
        vec3 viewDir = normalize(viewPosition - fragmentPosition);
    
        // == =====================================================
        // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
        // For each phase, a calculate function is defined that calculates the corresponding color
        // per light source. In the main() function we take all the calculated colors and sum them 
        // up for this fragment's final color.
        // == =====================================================
        // phase 1: directional lighting
        if(directionalLight.bActive == true)
        {
            phongResult += CalcDirectionalLight(directionalLight, norm, viewDir, surfaceColorLinear);
        }
        // phase 2: point lights
        for(int i = 0; i < TOTAL_POINT_LIGHTS; i++)
        {
	    if(pointLights[i].bActive == true)
            {
                phongResult += CalcPointLight(pointLights[i], norm, fragmentPosition, viewDir, surfaceColorLinear);   
            }
        } 
        // phase 3: spot light
        if(spotLight.bActive == true)
        {
            phongResult += CalcSpotLight(spotLight, norm, fragmentPosition, viewDir, surfaceColorLinear);    
        }
    
        if(bUseTexture == true)
        {
            outputColorLinear = vec4(phongResult, surfaceTextureLinear.a);
        }
        else
        {
            outputColorLinear = vec4(phongResult, objectColor.a);
        }
    }
    else
    {
        if(bUseTexture == true)
        {
            // Keep this path in linear space too, then convert once at the end.
            outputColorLinear = surfaceTextureLinear;
        }
        else
        {
            outputColorLinear = vec4(surfaceColorLinear, objectColor.a);
        }
    }

    fragmentColor = vec4(ApplyGamma(outputColorLinear.rgb), outputColorLinear.a);
}

vec4 SampleTextureLinear()
{
    vec4 texel = texture(objectTexture, fragmentTextureCoordinateScaled);
    texel.rgb = pow(max(texel.rgb, vec3(0.0f)), vec3(2.2f));
    return texel;
}

vec4 SampleDetailTextureLinear()
{
    vec4 texel = texture(detailTexture, detailTextureCoordinateScaled);
    texel.rgb = pow(max(texel.rgb, vec3(0.0f)), vec3(2.2f));
    return texel;
}

vec4 SampleSurfaceTextureLinear()
{
    vec4 baseTexel = SampleTextureLinear();
    if (bUseDetailTexture == false)
    {
        return baseTexel;
    }

    // Global multiplier lets us toggle detail blending at runtime for demo/testing.
    float blend = clamp(detailBlendFactor * detailBlendGlobal, 0.0f, 1.0f);
    if (blend <= 0.0001f)
    {
        return baseTexel;
    }

    // Keep the base alpha and blend only RGB for stable object edges.
    vec3 detailRgb = SampleDetailTextureLinear().rgb;
    vec3 blendedRgb = mix(baseTexel.rgb, detailRgb, blend);
    return vec4(blendedRgb, baseTexel.a);
}

vec3 GetObjectColorLinear()
{
    return pow(max(objectColor.rgb, vec3(0.0f)), vec3(2.2f));
}

vec3 ApplyGamma(vec3 linearColor)
{
    return pow(max(linearColor, vec3(0.0f)), vec3(1.0f / 2.2f));
}

// calculates the color when using a directional light.
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 surfaceColor)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 lightDirection = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDirection), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    ambient = light.ambient * surfaceColor;
    diffuse = light.diffuse * diff * material.diffuseColor * surfaceColor;
    specular = light.specular * spec * material.specularColor * surfaceColor;
    
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 surfaceColor)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular= vec3(0.0f);

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    // Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // Point lights fade with distance so fill/rim light stays local.
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
   
    // combine results
    ambient = light.ambient * surfaceColor;
    diffuse = light.diffuse * diff * material.diffuseColor * surfaceColor;
    specular = light.specular * specularComponent * material.specularColor;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 surfaceColor)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    ambient = light.ambient * surfaceColor;
    diffuse = light.diffuse * diff * material.diffuseColor * surfaceColor;
    specular = light.specular * spec * material.specularColor * surfaceColor;
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
