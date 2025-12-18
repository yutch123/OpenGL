#include <GL/glew.h> // должен идити первым, так как определяет различные используемые типы
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Window_OpenGL", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glfwSwapInterval(5);

    if (glewInit() != GLEW_OK)
        std::cout << "Error!" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    {

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

        unsigned int vao; // идентификатор объекта OpenGL
        GLCall(glGenVertexArrays(1, &vao)); // OpenGL создает 1 Vertex Array Object
        GLCall(glBindVertexArray(vao)); // Записывает этот ID в vao

        // Создаем объект VertexArray (VAO)
        // VAO хранит информацию о том, КАК интерпретировать вершинные данные
        VertexArray va;

        /* Создаём VertexBuffer (VBO)
        *  positions — массив float в CPU-памяти
        *  4 * 2 * sizeof(float) — размер буфера в байтах
        * (например: 4 вершины по 2 координаты: x, y)
        */
        VertexBuffer vb(positions, 4 * 2 * sizeof(float));

        /* Создаём объект BufferLayout
        *  Он описывает структуру ОДНОЙ вершины
        *  (какие атрибуты, их типы и размеры)
        */
        VertexBufferLayout layout;

        /* Добавляем в layout один атрибут:
        *  тип float, 3 компоненты (x, y, z)
        *  Это соответствует vec3 в вершинном шейдере
        */
        layout.Push<float>(2);

        /* Передаём layout в VAO
        *  Здесь вызываются glEnableVertexAttribArray и glVertexAttribPointer
        *  VAO запоминает, как читать данные из VBO
        */
        va.AddBuffer(vb, layout);

        IndexBuffer ib(indices, 6);

        ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
        unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
        GLCall(glUseProgram(shader));

        int location = glGetUniformLocation(shader, "u_Color"); // обращаемся к нашей униформе
        ASSERT(location != -1); // программа не нашла нашу униформу
        GLCall(glUniform4f(location, 0.9f, 0.3f, 0.8f, 1.0f)); // устанавливаем значение цвета

        GLCall(glBindVertexArray(0));
        GLCall(glUseProgram(0));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); // привязываем буфер элементов
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); // привязываем буфер элементов

        float r = 0.0f;
        float increment = 0.05f;

        // Игровой цикл
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            GLCall(glClear(GL_COLOR_BUFFER_BIT));

            GLCall(glUseProgram(shader)); // привязываем шейдер и вызываем массив отрисовки
            GLCall(glUniform4f(location, r, 0.3f, 0.8f, 1.0f));

            va.Bind();
            ib.Bind();

            GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr)); // отрисовка треугольника

            if (r > 1.0f)
                increment = -0.05f;
            else if (r < 0.0f)
                increment = 0.05f;

            r += increment;

            // Меняем буферы местами
            glfwSwapBuffers(window);


            // Функция glfwPollEvents проверяет были ли вызваны какие либо события (вроде ввода с клавиатуры или перемещение мыши) и вызывает установленные функции
            // (которые мы можем установить через функции обратного вызова (callback)).
            glfwPollEvents();
        }

        GLCall(glDeleteProgram(shader));
    }

    glfwTerminate(); // Как только мы вышли из игрового цикла, надо очистить выделенные нам ресурсы
    return 0;
}