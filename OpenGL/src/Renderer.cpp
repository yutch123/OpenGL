#include "Renderer.h"
#include <iostream>

void GLClearError()
{
    while (glGetError() != GL_NO_ERROR); // удаляем все предыдущие ошибки OpenGL

}

bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError()) // пока glGetError != 0
    {
        std::cout << "[OpenGL Error] " << error << function << " " << file << ":" << line << std::endl;
        return false; // вызов не удался
    }
    return true;
}