#version 330

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;

uniform int inv; 
uniform int redim; 

out vec2 vsoTexCoord;
out vec3 vsoNormal;
out vec3 vsoPosition;
out vec3 vsoModPos;

void main(void) 
{
  if(inv != 0)
    vsoTexCoord = vec2(vsiTexCoord.s, 1.0 - vsiTexCoord.t);
  else
    vsoTexCoord = vec2(vsiTexCoord.s, vsiTexCoord.t);
   
   
  vec4 mp = modelViewMatrix * vec4(vsiPosition, 1.0);
  vsoNormal = (transpose(inverse(modelViewMatrix))  * vec4(vsiNormal, 0.0)).xyz;
  vsoPosition = vsoPosition;
  vsoModPos   = mp.xyz;
  
  if(redim != 0)
    gl_Position = projectionMatrix * mp;
  else
    gl_Position = vec4(vsiPosition, 1.0);
}
