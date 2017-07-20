#version 440 core
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform float time;
uniform float m;
//uniform sampler2D backbuffer;

uniform sampler2D i0;
uniform sampler2D i1;
uniform sampler2D i2;
uniform sampler2D i3;
uniform float i0ratio;
uniform float i1ratio;
uniform float i2ratio;
uniform float i3ratio;

uniform float p0;
uniform float p1;
uniform float p3;
uniform float p4;

out vec4 fragColor;
float random (in float x) {
    return fract(sin(x)*43758.5453123)-0.5;
}

void main (void) {
	vec2 st = gl_FragCoord.xy/resolution.xy;
  vec4 img1 = texture2D(i0,st*vec2(i0ratio,1.));
  vec4 img2 = texture2D(i1,st*vec2(i1ratio,1.));
  vec4 img3 = texture2D(i2,st*vec2(i2ratio,1.));
  vec4 img4 = texture2D(i3,st*vec2(i3ratio,1.));
	img1 = mix(img1,img2,sin(time));
	img2 = mix(img3,img4,cos(time));
	fragColor = mix(img1,img2,p0);
	//gl_FragColor = i1;
}
