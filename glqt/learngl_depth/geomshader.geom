#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 5) out;
out vec3 fColor;

in VS_OUT {
    vec3 color;
} gs_in[];

void build_house(vec4 position)
{
    fColor = gs_in[0].color;
    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0);    // 1:左下
    EmitVertex();
    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0);    // 2:右下
    EmitVertex();
    gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0);    // 3:左上
    EmitVertex();
    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0);    // 4:右上
    EmitVertex();
    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0);    // 5:顶部
    EmitVertex();

    EndPrimitive();
}

void main() {
    // 这个只是简单的应用，实际几何着色器的一个功能就是爆破物体（炸弹成为碎片的过程），图元三角形沿着法向量移动，还有一个就是生成毛发，先正常渲染物体，再经过几何着色器渲染
    build_house(gl_in[0].gl_Position);
}
