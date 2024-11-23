#include <pch.h>
#include <scene.h>

#include <capture.h>
#include <shape.h>
#include <text.h>

#include <glm/gtc/matrix_transform.hpp>

extern std::vector<Window> windows;
extern GLuint texture;
extern GLuint white_texture;
extern Font font;
extern GLFWwindow* glfw_window;

struct Preview
{
    Frame frame;
    glm::vec2 position;
    Text caption;
    glm::vec2 caption_position;
};

static std::vector<Preview> previews;
static Window* selected = nullptr;
static unsigned int hovered = (unsigned int)-1;

static void calc_positions()
{
    static glm::vec2 gap = { 8, 8 };
    static glm::vec2 margin = { 16, 16 };
    static glm::vec2 frame_size = { 288, 168 };
    static glm::vec2 size = { frame_size.x, frame_size.y + 28 };

    int width, height;
    glfwGetWindowSize(glfw_window, &width, &height);

    previews.clear();

    unsigned int columns = (unsigned int)ceil(sqrtf((float)windows.size()));
    unsigned int rows = (unsigned int)ceil((float)windows.size() / columns);
    glm::vec2 grid = {
        margin.x * 2 + gap.x * (columns - 1) + size.x * columns,
        margin.y * 2 + gap.y * (rows - 1) + size.y * rows
    };
    unsigned int last_row_columns = (unsigned int)windows.size() - (columns * (rows - 1));
    float last_row_width = gap.x * (last_row_columns - 1) + size.x * last_row_columns;

    unsigned int column = 0, row = 0;
    for (Window& window : windows)
    {
        float x;
        if (column < columns) x = margin.x + (gap.x + size.x) * column++;
        else
        {
            row++;
            x = margin.x + (gap.x + size.x) * (column = 0)++;
        }
        if (row == rows - 1) x = (grid.x - last_row_width) * 0.5f + (gap.x + size.x) * (column - 1);
        float y = height - margin.y - gap.y * row - size.y * (row + 1);

        x += (width - grid.x) * 0.5f;
        y -= (height - grid.y) * 0.5f;

        Text caption = preview_text(&font, window.caption, 0.6f, size.x - 20, L"...");
        float tx = x + (size.x - caption.size.x) * 0.5f;
        float ty = y - 20;
        previews.push_back({ snapshot(&window), { x, y }, caption, { tx, ty } });
    }
}

static void preview_scene(float delta_time)
{
    static float refresh = 1.0f;
    if ((refresh += delta_time) >= 1.0f)
    {
        enum_windows();
        calc_positions();
        refresh = 0;
    }

    static glm::vec2 preview_size = glm::vec2(288, 168);
    for (unsigned int i = 0; i < previews.size(); i++)
    {
        Preview& preview = previews[i];
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, preview.frame.width, preview.frame.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, preview.frame.pixels.data());

        glm::mat4 background = glm::mat4(1.0f);
        background = glm::translate(background, glm::vec3(preview.position - glm::vec2(1, 1), 0));
        background = glm::scale(background, glm::vec3(preview_size + glm::vec2(2, 2), 0));

        glm::mat4 frame = glm::mat4(1.0f);
        float x = ((float)preview.frame.width / preview.frame.height) * preview_size.y;
        float y = preview_size.y;
        if (x > preview_size.x)
        {
            float factor = preview_size.x / x;
            x *= factor;
            y *= factor;
        }
        frame = glm::translate(frame, glm::vec3(preview.position.x + (preview_size.x - x) * 0.5f, preview.position.y + (preview_size.y - y) * 0.5f, 0));
        frame = glm::scale(frame, glm::vec3(x, y, 0));

        glBindTexture(GL_TEXTURE_2D, white_texture);
        if (hovered == i)
        {
            glm::mat4 highlight = glm::mat4(1.0f);
            highlight = glm::translate(highlight, glm::vec3(preview.position - glm::vec2(3, 3), 0));
            highlight = glm::scale(highlight, glm::vec3(preview_size + glm::vec2(6, 6), 0));
            render_quad(highlight, 0, { 1.0f, 0.0f, 0.0f, 1.0f });
        }
        render_quad(background, 0, { 0.0f, 0.0f, 0.0f, 1.0f });
        glBindTexture(GL_TEXTURE_2D, texture);
        render_quad(frame, texture, { 1.0f, 1.0f, 1.0f, 1.0f });

        render_text(&preview.caption, preview.caption_position, { 1.0f, 1.0f, 1.0f }, glm::mat4(1.0f));
    }
}

static void capture_scene(float delta_time)
{
    Frame frame = snapshot(selected);

    int width, height;
    glfwGetWindowSize(glfw_window, &width, &height);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.width, frame.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, frame.pixels.data());

    glm::mat4 model = glm::mat4(1.0f);
    float x = ((float)frame.width / frame.height) * height;
    float y = (float)height;
    if (x > width)
    {
        float factor = width / x;
        x *= factor;
        y *= factor;
    }
    model = glm::translate(model, glm::vec3((width - x) * 0.5f, (height - y) * 0.5f, 0));
    model = glm::scale(model, glm::vec3(x, y, 0));

    render_quad(model, texture, { 1.0f, 1.0f, 1.0f, 1.0f });
}

void render_scene(float delta_time)
{
    glClearColor(0.2f, 0.2f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (selected == nullptr) preview_scene(delta_time);
    else capture_scene(delta_time);
}

static bool within(glm::vec2 point, glm::vec2 min, glm::vec2 max)
{
    if (point.x < min.x || point.x > max.x) return false;
    if (point.y < min.y || point.y > max.y) return false;
    return true;
}

void scene_hover(GLFWwindow* window, double xpos, double ypos)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    ypos = height - ypos;
    hovered = (unsigned int)-1;
    for (unsigned int i = 0; i < previews.size(); i++)
    {
        const Preview& preview = previews[i];
        if (!within({ xpos, ypos }, preview.position, preview.position + glm::vec2(288, 168))) continue;
        hovered = i;
        break;
    }
}

void scene_click(GLFWwindow* window, int button, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    ypos = height - ypos;
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:
        if (selected != nullptr) break;
        for (const Preview& preview : previews)
        {
            if (!within({ xpos, ypos }, preview.position, preview.position + glm::vec2(288, 168))) continue;
            selected = preview.frame.window;
            break;
        }
        if (selected != nullptr) previews.clear();
        break;
    case GLFW_MOUSE_BUTTON_4:
        if (selected == nullptr) return;
        selected = nullptr;
        enum_windows();
        calc_positions();
        break;
    default:
        break;
    }
}

void scene_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        if (selected == nullptr) return;
        selected = nullptr;
        enum_windows();
        calc_positions();
        break;
    default:
        break;
    }
}

void scene_resize(glm::vec2 size)
{
    enum_windows();
    calc_positions();
}
