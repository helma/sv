#version 440 core
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform float time;
uniform float m;
//uniform sampler2D backbuffer;

uniform sampler2D img0;
uniform sampler2D img1;
uniform sampler2D img2;
uniform sampler2D img3;
uniform float img0ratio;
uniform float img1ratio;
uniform float img2ratio;
uniform float img3ratio;

out vec4 fragColor;
float random (in float x) {
    return fract(sin(x)*43758.5453123)-0.5;
}

void main (void) {
	vec2 st = gl_FragCoord.xy/resolution.xy;
  vec4 i1 = texture2D(img0,st*vec2(img0ratio,1.));
  vec4 i2 = texture2D(img1,st*vec2(img1ratio,1.));
  vec4 i3 = texture2D(img2,st*vec2(img2ratio,1.));
  vec4 i4 = texture2D(img3,st*vec2(img3ratio,1.));
	i1 = mix(i1,i2,sin(time));
	i2 = mix(i3,i4,cos(time));
	fragColor = mix(i1,i2,0.5);
	//gl_FragColor = i1;
}
