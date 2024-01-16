#version 330 core
out vec4 FragColor;
in vec2 texCord;

const float offset = 1.0 / 300.0;

uniform sampler2D texture1;

void main()
{
    //FragColor = texture(texture1, texCord);
    //FragColor = vec4(vec3(1.0 - texture(texture1, texCord)), 1.0);// 反相操作

    // 灰度效果
    /* FragColor = texture(texture1, texCord);
    //float average = (FragColor.r + FragColor.g + FragColor.b) / 3.0;
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;// 对绿色更敏感，可以稍微加权平均
    FragColor = vec4(average, average, average, 1.0);*/

    // 核效果，或者卷积矩阵，对纹理坐标周末的采用然后乘以系数（实现锐化或者模糊等效果）
    vec2 offsets[9] = vec2[](
            vec2(-offset,  offset), // 左上
            vec2( 0.0f,    offset), // 正上
            vec2( offset,  offset), // 右上
            vec2(-offset,  0.0f),   // 左
            vec2( 0.0f,    0.0f),   // 中
            vec2( offset,  0.0f),   // 右
            vec2(-offset, -offset), // 左下
            vec2( 0.0f,   -offset), // 正下
            vec2( offset, -offset)  // 右下
        );

    // 锐化核
        /*float kernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );*/

        // 模糊核
        /*float kernel[9] = float[](
            1.0 / 16, 2.0 / 16, 1.0 / 16,
            2.0 / 16, 4.0 / 16, 2.0 / 16,
            1.0 / 16, 2.0 / 16, 1.0 / 16
        );*/

        // 边缘检测
        float kernel[9] = float[](
            1.0, 1.0, 1.0,
           1.0, -8.0, 1.0,
           1.0, 1.0, 1.0
        );

        vec3 sampleTex[9];
        for(int i = 0; i < 9; i++)
        {
            sampleTex[i] = vec3(texture(texture1, texCord.st + offsets[i]));
        }
        vec3 col = vec3(0.0);
        for(int i = 0; i < 9; i++)
            col += sampleTex[i] * kernel[i];

        FragColor = vec4(col, 1.0);
}
