#include <GL/glew.h> // должен идити первым, так как определяет различные используемые типы
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

static void GLClearError()
{
    while (glGetError() != GL_NO_ERROR); // удаляем все предыдущие ошибки OpenGL

}

static bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError()) // пока glGetError != 0
    {
        std::cout << "[OpenGL Error] " << error << function << " " << file << ":" << line << std::endl;
        return false; // вызов не удался
    }
    return true;
}

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
        if (line.find("#shader") != std::string::npos) // содержит ли эта срочка пользовательский синтаксический токен "shader"
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
    GLCall(glShaderSource(id, 1, &src, nullptr));
    GLCall(glCompileShader(id));

    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
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

    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));
    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));

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
    window = glfwCreateWindow(640, 480, "Window_OpenGL", NULL, NULL);
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

    float positions[] = { // инициализируем массив
        -0.5f, -0.5f, // 0
         0.5f, -0.5f, // 1
         0.5f,  0.5f, // 2
        -0.5f,  0.5f // 3
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int buffer;
    GLCall(glGenBuffers(1, &buffer)); // Создаем буффер и получаем его ID
                              // пока это просто число, никакой памяти не выделено.

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer)); // Делаем этот буфер "активным" для цели GL_ARRAY_BUFFER.
                                           // Теперь все операции glBufferData / glVertexAttribPointer будут относиться именно к этому буферу.

    GLCall(glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), positions, GL_STATIC_DRAW)); // Копируем данные массива positions в видеопамять (в VBO).
                                                                                 // - GL_ARRAY_BUFFER указывает, куда копировать
                                                                                 // - 6 * sizeof(float) = размер данных (здесь 6 float → 12 байт)
                                                                                 // - positions — указатель на данные в RAM, откуда OpenGL заберёт копию
                                                                                 // - GL_STATIC_DRAW — подсказка (hint) драйверу: данные редко меняются

    GLCall(glEnableVertexAttribArray(0)); // Включаем атрибут №0 в VAO.
                                  // Это говорит OpenGL: "данный атрибут будет использовать данные из VBO".
                                  // Если не включить — шейдер не будет получать этот атрибут.

    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0)); // Описываем **формат данных**, которые будут читаться из VBO для атрибута №0.
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


    unsigned int ibo; // индексный буферный обмен
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW));

    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    GLCall(glUseProgram(shader));

/* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr)); // отрисовка треугольника
       
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}

