#include <pch.h>

#include <scene.h>
#include <shape.h>
#include <text.h>

GLuint texture;
GLuint white_texture;
Font font;
GLFWwindow* glfw_window;

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_POSITION_X, (int)((vidmode->width - 1280) >> 1));
    glfwWindowHint(GLFW_POSITION_Y, (int)((vidmode->height - 720) >> 1));

    glfw_window = glfwCreateWindow(1280, 720, "Screenshare", 0, 0);

    glfwMakeContextCurrent(glfw_window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(glfw_window, [](GLFWwindow* window, int width, int height)
        {
            glViewport(0, 0, width, height);
            resize_quad({ width, height });
            resize_font_projection({ width, height });
            scene_resize({ width, height });
        });
    glfwSetCursorPosCallback(glfw_window, scene_hover);
    glfwSetMouseButtonCallback(glfw_window, scene_click);
    glfwSetKeyCallback(glfw_window, scene_key);

    glfwSwapInterval(1);

    init_quad();
    init_font_renderer();

    font = load_font("C:/Windows/Fonts/arial.ttf", 24);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &white_texture);
    glBindTexture(GL_TEXTURE_2D, white_texture);

    unsigned char data[] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float last = (float)glfwGetTime();
    while (!glfwWindowShouldClose(glfw_window))
    {
        float current = (float)glfwGetTime();
        float delta = current - last;
        last = current;

        render_scene(delta);

        glfwSwapBuffers(glfw_window);
        glfwPollEvents();
    }

    destroy_quad();
    destroy_font_renderer();

    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    return 0;
}
