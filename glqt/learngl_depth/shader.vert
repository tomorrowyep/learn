#version 330 core
layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec2 aTexCord;
layout (location = 1) in vec3 aNormal;
//out vec2 texCord;
out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    /*可能会常用到的内置变量
      // 顶点着色器
        **gl_PointSize:调整点的大小，默认是禁用的，用函数glEnable(GL_PROGRAM_POINT_SIZE)开启，这样表示一个点就是一个图元
        *简单的例子：设置为深度值，可以从视觉层面来判断这个点离摄像机的远近，点越大越远
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        gl_PointSize = gl_Position.z;

        **gl_VertexID:为一个输入变量，整型变量gl_VertexID储存了正在绘制顶点的当前ID。当（使用glDrawElements）进行索引渲染的时候，这个变量会存储正在绘制顶点的当前索引。
                      当（使用glDrawArrays）不使用索引进行绘制的时候，这个变量会储存从渲染调用开始的已处理顶点数量，暂时没啥用。

      // 片段着色器
      **gl_FragCoord:输入变量且是一个只读变量，不能修改，记录了像素视图窗口的位置，z分量就等于深度值
      *简单例子：gl_FragCoord的一个常见用处是用于对比不同片段计算的视觉输出效果，x方向400为分界线，区别颜色
        if(gl_FragCoord.x < 400)
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        else
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);

       **gl_FrontFacing:是一个bool类型，可以用来判断是正向面还是背向面，然后可以用不同的纹理渲染，前提是不要开启面剔除，不然直接看不到
       *简单例子：根据这个参数判断是否是正向面来渲染不同的纹理
       if(gl_FrontFacing)
            FragColor = texture(frontTexture, TexCoords);
       else
            FragColor = texture(backTexture, TexCoords);

       **gl_FragDepth:如果着色器没有写入值到gl_FragDepth，它会自动取用gl_FragCoord.z的值，一般不用，会影响性能。gl_FragDepth = 0.0; // 这个片段现在的深度值为 0.0

       // 通用
        **接口块:方便统一从顶点着色器向片段着色器发送的数据
        简单例子：定义就好像结构体struct一样，这里使用out、in来声明
        顶点着色器的定义：
        out VS_OUT
        {
            vec2 TexCoords;
        } vs_out;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vs_out.TexCoords = aTexCoords;
        }
        片段着色器的定义：
        in VS_OUT
        {
            vec2 TexCoords;
        } fs_in;

        *注：定义的结构体要一样，名称随意

        **uniform缓冲对象:也是一个缓冲区，允许我们定义一系列在多个着色器中相同的全局Uniform变量。当使用Uniform缓冲对象的时候，我们只需要设置相关的uniform一次。
        *简单例子： layout (std140)声明布局方式（shered、packed），可以保证顺序不变，偏移量可以自己计算出来，类似字节对齐，下面的例子共152字节
        layout (std140) uniform ExampleBlock
        {
                             // 基准对齐量       // 对齐偏移量
            float value;     // 4               // 0
            vec3 vector;     // 16              // 16  (必须是16的倍数，所以 4->16)
            mat4 matrix;     // 16              // 32  (列 0)
                             // 16              // 48  (列 1)
                             // 16              // 64  (列 2)
                             // 16              // 80  (列 3)
            float values[3]; // 16              // 96  (values[0])
                             // 16              // 112 (values[1])
                             // 16              // 128 (values[2])
            bool boolean;    // 4               // 144
            int integer;     // 4               // 148
        };

        unsigned int uboExampleBlock;
        glGenBuffers(1, &uboExampleBlock);
        glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
        glBufferData(GL_UNIFORM_BUFFER, 152, NULL, GL_STATIC_DRAW); // 分配152字节的内存
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        *注：GLSL中的每个变量，比如说int、float和bool，都被定义为4字节量

        *如何使用：
        1、绑定shader的uniform块到uniform缓冲对象的链接点，不同的shader可以绑定到同一个uniform缓冲对象的链接点，前提是使用了相同的uniform块
        2、绑定uniform缓冲对象到对应的链接点，这个是唯一的
        3、写入对应的数据，更新也是一样的，更新一次全部共用

        简单例子：
        1、
        unsigned int lights_index = glGetUniformBlockIndex(shaderA.ID, "Lights");
        glUniformBlockBinding(shaderA.ID, lights_index, 2);

        2、
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboExampleBlock);
        或
        glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboExampleBlock, 0, 152);

        3、
        glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
        int b = true; // GLSL中的bool是4字节的，所以我们将它存为一个integer
        glBufferSubData(GL_UNIFORM_BUFFER, 144, 4, &b);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    */
    Normal = mat3(transpose(inverse(model))) * aNormal;// 去掉平移效果
    Position = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    //texCord = aTexCord;
}
