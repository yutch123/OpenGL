#include <GL/glew.h> // должен идити первым, так как определяет различные используемые типы
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

static ShaderProgramSource ParseShader(const std::string& filepath) // парсим наш шейдер
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line; // line содержит нашу фактическую строку
    std::stringstream ss[2]; // создаем 2 разных строковых потока
    ShaderType type = ShaderType::NONE;
    // будем просматривать файл прострочно
    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos) // содержит ли эта срочка пользовательский синаксический токен "shader"
        {
            // определем тип шейдера
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }

    return { ss[0].str(), ss[1].str() };
}

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
        
    return id;
}

static int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) // функция для создания шейдера
{
    // цель функции - сделать несколько вещей, но OpenGl нужно предоставить наш исходный код шейдера
    unsigned int program = glCreateProgram();
    // создаем шейдеры
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
        std::cout << "Error!" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    float positions[6] = { // инициализируем массив
        -0.5f, -0.5f,
         0.0f,  0.5f,
         0.5f, -0.5f,
    };

    unsigned int buffer;
    glGenBuffers(1, &buffer); // Создаем буффер и получаем его ID
                              // пока это просто число, никакой памяти не выделено.

    glBindBuffer(GL_ARRAY_BUFFER, buffer); // Делаем этот буфер "активным" для цели GL_ARRAY_BUFFER.
                                           // Теперь все операции glBufferData / glVertexAttribPointer будут относиться именно к этому буферу.

    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), positions, GL_STATIC_DRAW); // Копируем данные массива positions в видеопамять (в VBO).
                                                                                 // - GL_ARRAY_BUFFER указывает, куда копировать
                                                                                 // - 6 * sizeof(float) = размер данных (здесь 6 float → 12 байт)
                                                                                 // - positions — указатель на данные в RAM, откуда OpenGL заберёт копию
                                                                                 // - GL_STATIC_DRAW — подсказка (hint) драйверу: данные редко меняются

    glEnableVertexAttribArray(0); // Включаем атрибут №0 в VAO.
                                  // Это говорит OpenGL: "данный атрибут будет использовать данные из VBO".
                                  // Если не включить — шейдер не будет получать этот атрибут.

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0); // Описываем **формат данных**, которые будут читаться из VBO для атрибута №0.
                                                                           // Параметры:
                                                                           // (index = 0) — номер атрибута в шейдере (layout(location = 0))
                                                                           // (size = 2) — атрибут состоит из 2 компонентов (например, vec2: x, y)
                                                                           // (type = GL_FLOAT) — каждый компонент — float
                                                                           // (normalized = GL_FALSE) — float НЕ нормализуем (нормализация нужна для целочисленных типов)
                                                                           // (stride = sizeof(float) * 2) — расстояние между началом двух последовательных вершин:
                                                                           // [x y] [x y] [x y] ...
                                                                           //  Каждая вершина занимает 8 байт (2 float)
                                                                           //  Поэтому stride = 8
                                                                           // (pointer = 0) — смещение внутри VBO, откуда начинается первый атрибут.
                                                                           //  0 означает "начинать прямо с начала данных"
                                                                           //  Это БАЙТОВОЕ смещение, не указатель на CPU-данные.


    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    glUseProgram(shader);

/* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        // отрисовка треугольника
        glDrawArrays(GL_TRIANGLES, 0, 3);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}

