#version 330

uniform sampler2D myTexture;
in  vec2 vsoTexCoord;
out vec4 fragColor;

void main(void) {
  fragColor = texture(myTexture, vsoTexCoord.st);
}
