#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform float time;
uniform float m;
//uniform sampler2D backbuffer;

uniform sampler2D image1;
uniform sampler2D image2;
uniform sampler2D image3;
//uniform vec2 u_tex0Resolution;

float random (in float x) {
    return fract(sin(x)*43758.5453123)-0.5;
}

void main (void) {
	vec2 st = gl_FragCoord.xy/resolution.xy;
  vec4 i1 = texture2D(image1,st*0.8);
  vec4 i2 = texture2D(image2,st);
	gl_FragColor = mix(i1,i2,m);
  //gl_FragColor = texture2D(image1,st*sin(time*2.));
}
