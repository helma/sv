#version 440 core
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform float time;

uniform sampler2D images[4];
uniform float ratios[4];
uniform float params[32];

out vec4 fragColor;

float random (in float x) {
  return fract(sin(x)*43758.5453123)-0.5;
}

vec4 image(int i) {
	vec2 st = gl_FragCoord.xy/resolution.xy;
  st.x *= ratios[i];
  st.x += (1-ratios[i])*0.5;
  return texture2D(images[i],st);
}

void main (void) {
	vec4 img1 = mix(image(0),image(1),sin(time));
	vec4 img2 = mix(image(2),image(3),cos(time));
  fragColor = mix(img1,img2,params[1]);
}
