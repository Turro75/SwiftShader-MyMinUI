#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <SDL2/SDL.h>
  
const char *vShaderSrc =
    "attribute vec4 a_position;   \n"
    "attribute vec2 a_texCoord;   \n"
    "varying vec2 v_texCoord;     \n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = a_position; \n"
    "   v_texCoord = a_texCoord;  \n"
    "}                            \n";
  
const char *fShaderSrc =
    "#ifdef GL_ES                                              \n"
    "precision mediump float;                                  \n"
    "#endif                                                    \n"
    "varying vec2 v_texCoord;                                  \n"
    "uniform float angle;                                      \n"
    "uniform float aspect;                                     \n"
    "uniform sampler2D s_texture;                              \n"
    "const vec2 HALF = vec2(0.5);                              \n"
    "void main()                                               \n"
    "{                                                         \n"
    "    float aSin = sin(angle);                              \n"
    "    float aCos = cos(angle);                              \n"
    "    vec2 tc = v_texCoord;                                 \n"
    "    mat2 rotMat = mat2(aCos, -aSin, aSin, aCos);          \n"
    "    mat2 scaleMat = mat2(aspect, 0.0, 0.0, 1.0);          \n"
    "    mat2 scaleMatInv = mat2(1.0 / aspect, 0.0, 0.0, 1.0); \n"
    "    tc -= HALF.xy;                                        \n"
    "    tc = scaleMatInv * rotMat * scaleMat * tc;            \n"
    "    tc += HALF.xy;                                        \n"
    "    vec3 tex = texture2D(s_texture, tc).rgb;              \n"
    "    gl_FragColor = vec4(tex, 1.0);                        \n"
    "}                                                         \n";
 
int main(int argc, char **argv)
{
    int w = 320;
    int h = 240;
  
    SDL_Window *window = NULL;
    SDL_GLContext context = {0};
  
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  
    window = SDL_CreateWindow("main", 0, 0, w, h, SDL_WINDOW_OPENGL);
    context = SDL_GL_CreateContext(window);
  
    GLuint vShader = 0;
    GLuint fShader = 0;
    GLuint pObject = 0;
  
    vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vShaderSrc, NULL);
    glCompileShader(vShader);
    GLint compiled = 0;
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint len = 0;
        glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) { 
            char* info = malloc(sizeof(char) * len);
            glGetShaderInfoLog(vShader, len, NULL, info);
            printf("failed to compile vShader: %s\n", info);
            free(info);
        }
    }
 
    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fShaderSrc, NULL);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint len = 0;
        glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            char* info = malloc(sizeof(char) * len);
            glGetShaderInfoLog(fShader, len, NULL, info);
            printf("failed to compile fShader: %s\n", info);
            free(info);
        }   
    }
 
    pObject = glCreateProgram();
    glAttachShader(pObject, vShader);
    glAttachShader(pObject, fShader);
    glLinkProgram(pObject);
    glUseProgram(pObject);
  
    GLint positionLoc = glGetAttribLocation(pObject, "a_position");
    GLint texCoordLoc = glGetAttribLocation(pObject, "a_texCoord");
    GLint samplerLoc = glGetUniformLocation(pObject, "s_texture");
    glUniform1f(glGetUniformLocation(pObject, "angle"), 90.0 * (3.1415 * 2.0) / 360.0);
    glUniform1f(glGetUniformLocation(pObject, "aspect"), (float)w / h);
      
    GLuint textureId = 0;
    GLubyte pixels[320 * 240 * 3] = {0};
  
    int x = 0, y = 0;
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            switch (y / 80) {
            case 0:
                pixels[(y * 320 * 3) + (x * 3) + 0] = 0xff;
                pixels[(y * 320 * 3) + (x * 3) + 1] = 0x00;
                pixels[(y * 320 * 3) + (x * 3) + 2] = 0x00;
                break;
            case 1:
                pixels[(y * 320 * 3) + (x * 3) + 0] = 0x00;
                pixels[(y * 320 * 3) + (x * 3) + 1] = 0xff;
                pixels[(y * 320 * 3) + (x * 3) + 2] = 0x00;
                break;
            case 2:
                pixels[(y * 320 * 3) + (x * 3) + 0] = 0x00;
                pixels[(y * 320 * 3) + (x * 3) + 1] = 0x00;
                pixels[(y * 320 * 3) + (x * 3) + 2] = 0xff;
                break;
            }
        }
    }
  
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  
    GLfloat vVertices[] = {
       -1.0f,  1.0f, 0.0f, 0.0f,  0.0f,
       -1.0f, -1.0f, 0.0f, 0.0f,  1.0f,
        1.0f, -1.0f, 0.0f, 1.0f,  1.0f,
        1.0f,  1.0f, 0.0f, 1.0f,  0.0f
    };
    GLushort indices[] = {0, 1, 2, 0, 2, 3};
  
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3]);
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(texCoordLoc);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(samplerLoc, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
    SDL_GL_SwapWindow(window);
    SDL_Delay(30000);
  
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}
