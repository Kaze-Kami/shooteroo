/**
  * Created by Joscha Vack on 2/20/2020.
  *
  **/


#include "WindowsWindow.h"

#include <glad/glad.h>

#include "Macros.h"
#include "core/logger/Logger.h"

#include "core/application/Application.h"
#include "core/events/MouseEvent.h"
#include "core/events/KeyEvent.h"

namespace Engine {

    static bool glfwInitialized = false;

    static void GLFWErrorCallback(int error, const char* description) {
        LOG_ERROR("GLFW Error ({}: {})", error, description);
    }

    Window* Window::create(const WindowProps& props, const EventCallbackFn& callback) {
        return new WindowsWindow(props, callback);
    }

    WindowsWindow::WindowsWindow(const WindowProps& props, const EventCallbackFn& callback) {
        init(props, callback);
    }

    WindowsWindow::~WindowsWindow() {
        shutdown();
    }

    void WindowsWindow::init(const WindowProps& props, const EventCallbackFn& callback) {
        data.name = props.name;
        data.eventCallback = callback;

        if (!glfwInitialized)
        {
            int glfwStatus = glfwInit();
            CORE_ASSERT(glfwStatus, "Could not intialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
            glfwInitialized = true;
        }

        // hide window till creation
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        // set up window
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        int width, height;

        if (props.fullScreen) {
            width = videoMode->width;
            height = videoMode->height;

            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            LOG_INFO("Window ({}:{}, Fullscreen)", width, height);
        }
        else {
            width = props.width;
            height = props.height;
            LOG_INFO("Window ({}:{})", width, height);
        }

        if (props.multisample) {
            LOG_INFO("Multisample (x{})", props.multisample);
            glfwWindowHint(GLFW_SAMPLES, props.multisample);
        }

        data.width = width;
        data.height = height;
        window = glfwCreateWindow(width, height, data.name.c_str(), nullptr, nullptr);

        if (!props.fullScreen) {
            glfwSetWindowPos(window, (videoMode->width - props.width) / 2, (videoMode->height - props.height) / 2);
        }

        glfwMakeContextCurrent(window);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        CORE_ASSERT(status, "Failed to initialize Glad!");

        LOG_INFO("OpenGL Info:");
        LOG_INFO("  Vendor: {}", glGetString(GL_VENDOR));
        LOG_INFO("  Renderer: {}", glGetString(GL_RENDERER));
        LOG_INFO("  Version: {}", glGetString(GL_VERSION));

        if (props.multisample) {
            LOG_INFO("Enabling gl multisample");
            glEnable(GL_MULTISAMPLE);
        }

        glfwSetWindowUserPointer(window, &data);
        setVsync(props.vsync);

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.width = width;
            data.height = height;

            WindowResizeEvent event(width, height);
            data.eventCallback(event);
        });

        glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            data.eventCallback(event);
        });

        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(key, 0);
                    data.eventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(key);
                    data.eventCallback(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(key, 1);
                    data.eventCallback(event);
                    break;
                }
            }
        });

        glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int keycode)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            KeyTypedEvent event(keycode);
            data.eventCallback(event);
        });

        glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button);
                    data.eventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button);
                    data.eventCallback(event);
                    break;
                }
            }
        });

        glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            MouseScrolledEvent event((float)xOffset, (float)yOffset);
            data.eventCallback(event);
        });

        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            MouseMovedEvent event((float)xPos, (float)yPos);
            data.eventCallback(event);
        });

        glfwShowWindow(window);
    }

    void WindowsWindow::shutdown() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void WindowsWindow::onUpdate() {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void WindowsWindow::setVsync(bool enabled) {
        data.vsync = enabled;

        if (enabled) {
            glfwSwapInterval(1);
            LOG_INFO("Vsync on");
        }
        else {
            glfwSwapInterval(0);
            LOG_INFO("Vsync off");
        }
    }

    bool WindowsWindow::isVsync() const {
        return data.vsync;
    }

}