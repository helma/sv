#version 330 core
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
//uniform vec2 u_tex0Resolution;

float random (in float x) {
    return fract(sin(x)*43758.5453123)-0.5;
}

void main (void) {
	vec2 st = gl_FragCoord.xy/resolution.xy;
  vec4 i1 = texture2D(img0,st*0.8);
  vec4 i2 = texture2D(img1,st);
  vec4 i3 = texture2D(img2,st*1.2);
  vec4 i4 = texture2D(img3,st);
	i1 = mix(i1,i2,sin(time));
	i2 = mix(i3,i4,cos(time));
	gl_FragColor = mix(i1,i2,0.5);
	//gl_FragColor = i2;
  //gl_FragColor = texture2D(image1,st*sin(time*2.));
}
