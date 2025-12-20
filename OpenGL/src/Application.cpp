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

#include "Shader.h"

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


        Shader shader("res/shaders/Basic.shader");
        shader.Bind();
        shader.SetUniform4f("u_Color", 0.9f, 0.3f, 0.8f, 1.0f);

        va.Unbind();
        vb.Unbind();
        ib.Unbind();
        shader.UnBind();

        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); // привязываем буфер элементов
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); // привязываем буфер элементов

        float r = 0.0f;
        float increment = 0.05f;

        // Игровой цикл
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            GLCall(glClear(GL_COLOR_BUFFER_BIT));

            shader.Bind();
            shader.SetUniform4f("u_Color", r, 0.3f, 0.8f, 1.0f);

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
    }

    glfwTerminate(); // Как только мы вышли из игрового цикла, надо очистить выделенные нам ресурсы
    return 0;
}