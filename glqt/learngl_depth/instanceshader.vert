#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;
out vec3 fColor;

uniform vec2 offsets[100];

void main()
{
    /*1、使用gl_InstanceID，有局限性，当数量很多的时候会超出uiniform数组的元素个数上限
    vec2 offset = offsets[gl_InstanceID];
    gl_Position = vec4(aPos + offset, 0.0, 1.0);
    fColor = aColor;*/

    //2、避免方法1的局限性，使用顶点属性的方法来传递，这样能够传递的数量就完全可以支撑业务了
    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
