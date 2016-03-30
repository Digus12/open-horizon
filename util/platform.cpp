//
// open horizon -- undefined_darkness@outlook.com
//

#include "platform.h"
#include "GLFW/glfw3.h"
#include "render/render.h"

#ifdef __APPLE__
    #include <Cocoa/Cocoa.h>
#endif

//------------------------------------------------------------

std::map<int, bool> platform::m_buttons;
platform::key_callback platform::m_key_callback;

//------------------------------------------------------------

bool platform::init(int width, int height, const char *title)
{
    if (!glfwInit())
        return false;

    if (nya_render::get_render_api() == nya_render::render_api_opengl3)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    glfwSwapInterval(1);

    /*
#ifdef _WIN32
        typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT)
            wglSwapIntervalEXT(1);
#endif
    */

    m_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(m_window, key_func);
    glfwMakeContextCurrent(m_window);
    glfwGetFramebufferSize(m_window, &m_screen_w, &m_screen_h);

    return true;
}

//------------------------------------------------------------

void platform::terminate()
{
    if (!m_window)
        return;

    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_window = 0;
}

//------------------------------------------------------------

bool platform::should_terminate()
{
    return m_window ? glfwWindowShouldClose(m_window) > 0 : true;
}

//------------------------------------------------------------

void platform::end_frame()
{
    if (!m_window)
        return;

    glfwSwapBuffers(m_window);
    m_last_buttons = m_buttons;
    glfwPollEvents();

    double mx,my;
    glfwGetCursorPos(m_window, &mx, &my);
    m_mouse_x = int(mx), m_mouse_y = int(my);
    glfwGetFramebufferSize(m_window, &m_screen_w, &m_screen_h);
}

//------------------------------------------------------------

bool platform::was_pressed(int key)
{
    auto k = m_buttons.find(key);
    if (k == m_buttons.end() || !k->second)
        return false;

    auto l = m_last_buttons.find(key);
    return l == m_last_buttons.end() || !l->second;
}

//------------------------------------------------------------

std::string platform::open_folder_dialog()
{
#ifdef __WIN32

    //ToDo

#elif __APPLE__
    NSArray *fileTypes = [NSArray arrayWithObjects:nil];
    NSOpenPanel * panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseDirectories:YES];
    [panel setCanChooseFiles:NO];
    [panel setFloatingPanel:YES];
    NSInteger result = [panel runModalForDirectory:NSHomeDirectory() file:nil
                                             types:fileTypes];
    if(result == NSOKButton)
    {
        NSArray *urls = [panel URLs];
        if ([urls count] < 1)
            return "";

        NSURL *url = [urls objectAtIndex:0];
        return std::string([url path].UTF8String) + "/";
    }
#else

    //ToDo: detect qt and load dynamically?

#endif

    return "";
}

//------------------------------------------------------------

bool platform::open_msgbox(std::string caption, std::string message)
{
    //ToDo

    return false;
}

//------------------------------------------------------------

bool platform::get_key(int key) { auto k = m_buttons.find(key); return k == m_buttons.end() ? false : k->second; }
bool platform::get_mouse_lbtn() { return m_window ? glfwGetMouseButton(m_window, 0) > 0 : false; }
bool platform::get_mouse_rbtn() { return m_window ? glfwGetMouseButton(m_window, 1) > 0 : false; }
int platform::get_mouse_x() { return m_mouse_x; }
int platform::get_mouse_y() { return m_mouse_y; }
int platform::get_width() { return m_screen_w; }
int platform::get_height() { return m_screen_h; }

//------------------------------------------------------------

void platform::set_keyboard_callback(key_callback &k) { m_key_callback = k; }

//------------------------------------------------------------

void platform::key_func(GLFWwindow *, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        m_buttons[key] = true;
        if (m_key_callback)
        {
            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
                m_key_callback('0' + key - GLFW_KEY_0);
            else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
                m_key_callback('A' + key - GLFW_KEY_A);
            else if (key == GLFW_KEY_PERIOD)
                m_key_callback('.');
            else if (key == GLFW_KEY_SEMICOLON)
                m_key_callback(':');
            else if (key == GLFW_KEY_MINUS)
                m_key_callback('-');
            else if (key == GLFW_KEY_SLASH)
                m_key_callback('/');
            else if (key == GLFW_KEY_SPACE)
                m_key_callback(' ');
            else if (key == GLFW_KEY_BACKSPACE)
                m_key_callback(8);
        }
    }
    else if (action == GLFW_RELEASE)
        m_buttons[key] = false;
}

//------------------------------------------------------------
