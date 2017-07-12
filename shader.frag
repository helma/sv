#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform float time;
uniform float color;

float box(in vec2 _st, in vec2 _size){
    _size = vec2(0.5) - _size*0.5;
    vec2 uv = smoothstep(_size,
                        _size+vec2(0.001),
                        _st);
    uv *= smoothstep(_size,
                    _size+vec2(0.001),
                    vec2(1.0)-_st);
    return uv.x*uv.y;
}

float cross(in vec2 _st, float _size){
    return  box(_st, vec2(_size,_size/4.)) + 
            box(_st, vec2(_size/4.,_size));
}

void main(){
    vec2 st = gl_FragCoord.xy/resolution.xy;
    vec3 c = vec3(0.);

    c += vec3(cross(st,0.5*abs(cos(time))));
    c.r -= color;
    //c *= u_color;

    gl_FragColor = vec4(c,1.0);
}
