layout(location = 0) in vec2 a_vec;

out vec2 position;

void main()
{
    gl_Position = vec4(a_vec, 0, 1);
    position = vec2(gl_Position);
}
