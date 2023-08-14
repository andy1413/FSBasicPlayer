precision highp float;

varying highp vec2 fsh_texcoord;

attribute highp vec4 vsh_position;
attribute highp vec2 vsh_texcoord;

uniform mat4 vsh_matrix;

void main() {
    gl_Position  = vsh_matrix * vsh_position;
    fsh_texcoord = vsh_texcoord;
}
