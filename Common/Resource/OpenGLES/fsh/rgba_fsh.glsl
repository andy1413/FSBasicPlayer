precision highp float;

uniform highp sampler2D sampler;
varying highp vec2 fsh_texcoord;

void main() {
    highp vec3 rgb = texture2D(sampler, fsh_texcoord).rgb;
    
    gl_FragColor = vec4(rgb.r, rgb.g, rgb.b, 1.0);
}
