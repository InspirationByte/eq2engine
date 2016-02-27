#include <string>
#include <sstream>
//#include <chrono> /* C++11 functionality */
//#include <random>
#include <cstdlib>

#include <SDL.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/*
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
*/
/* Shader sources */
const GLchar* vertexSource =
    "attribute vec4 vPosition;"
    "void main() "
    "{"
    "   gl_Position = vPosition;"
    "}";

const GLchar* fragmentSource =
    "precision mediump float;"
    "void main()"
    "{"
    "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); "
    "}";

int width = 0;
int height = 0;


void Draw( void )
{
    GLfloat vVertices[] = { 0.0f, 0.5f, 0.0f,
                            -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f, 0.0f};

    // Clear the color buffer
    glViewport( 0, 0, width, height );
    glClear( GL_COLOR_BUFFER_BIT );

    // Load the vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main(int argc, char** argv)
{
    /* Initialize SDL library */
    SDL_Window* sdlWindow = 0;
    SDL_GLContext sdlGL = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to initalize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // PDS: Sets OpenGL ES 3.0 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); // PDS: Sets OpenGL ES 2.0 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_DisplayMode mode;
    SDL_GetDisplayMode(0, 0, &mode);
    width = mode.w;
    height = mode.h;

    SDL_Log("Width = %d. Height = %d\n", width, height);

    sdlWindow = SDL_CreateWindow(NULL, 0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);

    if (sdlWindow == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create the sdlWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }



    sdlGL = SDL_GL_CreateContext(sdlWindow);

    /* Query OpenGL device information */
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major = 0, minor = 0;
    //glGetIntegerv(GL_MAJOR_VERSION, &major);
    //glGetIntegerv(GL_MINOR_VERSION, &minor);

    std::stringstream ss;
    ss << "\n-------------------------------------------------------------\n";
    ss << "GL Vendor    : " << vendor;
    ss << "\nGL GLRenderer : " << renderer;
    ss << "\nGL Version   : " << version;
    ss << "\nGL Version   : " << major << "." << minor;
    ss << "\nGLSL Version : " << glslVersion;
    ss << "\n-------------------------------------------------------------\n";
    SDL_Log(ss.str().c_str());

    /* Create and compile the vertex shader */
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);

    GLint nRC = 0;
    glCompileShader(vertexShader);

    glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &nRC );

    SDL_Log( "PDS: Compile(A) status: %d", nRC );

    /* Create and compile the fragment shader */
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &nRC );

    SDL_Log( "PDS: Compile(B) status: %d", nRC );

    /* Link the vertex and fragment shader into a shader program */
    GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    // Bind vPosition to attribute 0
    glBindAttribLocation(shaderProgram, 0, "vPosition");

    glLinkProgram(shaderProgram);

    // Check the link status
    GLint linked;

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);

    if(!linked)
    {
        SDL_Log( "Link failure\n" );
        GLint infoLen = 0;
        if(infoLen > 1)
        {
            char txTmp[ 8192 ];

            char* infoLog = (char*) malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(shaderProgram, infoLen, NULL, infoLog);
            sprintf( txTmp, "Error linking program:\n%s\n", infoLog);
            SDL_Log( txTmp );

            free(infoLog);
        }
        glDeleteProgram(shaderProgram);
        return 0;
    }

    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    bool done = false;

    while( ! done )
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                done = true;
            }
            else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION || event.type == SDL_FINGERMOTION)
            {
                // no implementation right now
            }
        }

        Draw();

        // Swap OpenGL render buffers
        SDL_GL_SwapWindow(sdlWindow);
    } /* while !done */

    glDeleteProgram(shaderProgram);

    return EXIT_SUCCESS;
} /* main */
