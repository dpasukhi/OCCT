#version 450

layout(std140, set=0, binding=0) uniform Matrixes
{
  mat4 occWorldViewMatrix;  //!< World-view  matrix
  mat4 occProjectionMatrix; //!< Projection  matrix
  mat4 occModelWorldMatrix; //!< Model-world matrix
};

layout(std140, set=1, binding=0) uniform Colors
{
  vec4 uColor;
};

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec4 PositionWorld;
layout(location = 2) in vec4 Position;
layout(location = 3) in vec3 View;

layout(location = 0) out vec4 occFragColor0;

vec3 Ambient;
vec3 Diffuse;
vec3 Specular;
void directionalLightFirst (in vec3 theNormal,
                            in vec3 theView,
                            in bool theIsFront)
{
  vec3 aLight = normalize (vec3 (0.0, 0.0, 1.0));
  //if (occLight_IsHeadlight (0) == 0)
  {
    //aLight = vec3 (occWorldViewMatrix * vec4 (aLight, 0.0));
  }

  vec3 aHalf = normalize (aLight + theView);

  vec3  aFaceSideNormal = theIsFront ? theNormal : -theNormal;
  float aNdotL = max (0.0, dot (aFaceSideNormal, aLight));
  float aNdotH = max (0.0, dot (aFaceSideNormal, aHalf ));

  float aSpecl = 0.0;
  if (aNdotL > 0.0)
  {
    aSpecl = pow (aNdotH, 128.0 * 0.65);
  }

  Diffuse  += vec3(1.0, 1.0, 1.0) * aNdotL;
  Specular += vec3(1.0, 1.0, 1.0) * aSpecl;
}

vec4 computeLighting (in vec3 theNormal,
                      in vec3 theView,
                      in vec4 thePoint,
                      in bool theIsFront)
{
  Ambient  = vec3 (1.0, 1.0, 1.0);
  Diffuse  = vec3 (0.0);
  Specular = vec3 (0.0);
  vec3 aPoint = thePoint.xyz / thePoint.w;
    directionalLightFirst(theNormal, theView, theIsFront);
  vec4 aMatAmbient  = vec4 (0.329, 0.224, 0.027, 1.0);
  vec4 aMatDiffuse  = vec4 (0.780, 0.569, 0.114, 1.0);
  vec4 aMatSpecular = vec4 (0.992, 0.941, 0.808, 1.0);
  vec4 aMatEmission = vec4 (0.0, 0.0, 0.0, 0.0);
  vec3 aColor = Ambient  * aMatAmbient.rgb
              + Diffuse  * aMatDiffuse.rgb
              + Specular * aMatSpecular.rgb
                         + aMatEmission.rgb;
  return vec4 (aColor, aMatDiffuse.a);
}

void main()
{
  occFragColor0 = computeLighting (normalize (Normal), normalize (View), Position, gl_FrontFacing);;
}
