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

layout(location = 0) in vec4 occVertex;

layout(location = 0) out vec4 PositionWorld;
layout(location = 1) out vec4 Position;
layout(location = 2) out vec3 View;

void main()
{
  PositionWorld = occModelWorldMatrix * occVertex;
  Position      = occWorldViewMatrix * PositionWorld;
  View          = vec3 (0.0, 0.0, 1.0);
  gl_Position = occProjectionMatrix * occWorldViewMatrix * occModelWorldMatrix * occVertex;
}
