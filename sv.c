#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int width = 1920;
int height = 1080;
GLFWwindow* window;
GLuint vertex;
GLuint fragment;
GLuint shaderProgram;

typedef struct Shad {
  GLuint id;
  char vertex[40];
  char fragment[40];
  time_t mtime;
  int new;
} Shader;

Shader shader;

typedef struct Img {
  GLuint id;
  char path[40];
  float ratio;
  int new;
} Image;

Image images[4];

typedef struct Param {
  //char name[3];
  float value;
  int new;
} Parameter;

Parameter parameters[32];

static void error_callback(int error, const char* description) { fputs(description, stderr); }

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
}

const char * readShader(char * path) {
  FILE *file = fopen(path, "r");
  if (!file) { fprintf(stderr,"Cannot read %s\n",path); }
  char source[2000];
  int res = fread(source,1,2000-1,file);
  source[res] = 0;
  fclose(file);
  const char *c_str = source;
  return c_str;
}

GLuint compileShader(const char * source, GLenum type) {
  GLuint sh = glCreateShader(type);
  glShaderSource(sh, 1, &source, NULL);
  glCompileShader(sh);
  int success;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(sh, 512, NULL, infoLog);
    fprintf(stderr,"Shader compilation failed: %s\n",infoLog);
  }
  else { return sh; }
}

GLuint linkShader(GLuint vertex, GLuint fragment) {
  glUseProgram(0);
  glDeleteProgram(shader.id);
  shader.id = glCreateProgram();
  glAttachShader(shader.id, vertex);
  glAttachShader(shader.id, fragment);
  glLinkProgram(shader.id);
  int success;
  glGetProgramiv(shader.id, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shader.id, 512, NULL, infoLog);
    fprintf(stderr,"Shader linking failed: %s\n",infoLog);
  }
  glDetachShader(shader.id, vertex); 
  glDetachShader(shader.id, fragment); 
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void createShader() {
  vertex = compileShader(readShader(shader.vertex), GL_VERTEX_SHADER);
  fragment = compileShader(readShader(shader.fragment),GL_FRAGMENT_SHADER);
  linkShader(vertex,fragment);
  struct stat file_stat;
  stat(shader.fragment, &file_stat);
  shader.mtime = file_stat.st_mtime;
  glUseProgram(shader.id);
};

void createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  window = glfwCreateWindow(width,height, "", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetErrorCallback(error_callback);
  glewInit();
}

void uniform1f(char * var, float f) {
  int v = glGetUniformLocation(shader.id, var);
  glUniform1f(v, f);
}

void uniform2f(char * var, float f0, float f1) {
  int v = glGetUniformLocation(shader.id, var);
  glUniform2f(v,f0,f1);
}

void imageUniforms(int i) {
  //Image image = images[i];
  //char name[2];
  //sprintf(name,"i%i",i);
  glUniform1i(glGetUniformLocation(shader.id, "images")+i,i);
  //char ratioName[7];
  //sprintf(ratioName,"%sratio",name);
  glUniform1f(glGetUniformLocation(shader.id, "ratios")+i,images[i].ratio);
}

void readImage(int i) {

  images[i].new = 1;

  int w,h,comp;
  stbi_set_flip_vertically_on_load(1);
  unsigned char* pixels = stbi_load(images[i].path, &w, &h, &comp, STBI_rgb_alpha);

  images[i].ratio = (float) w/h;
  imageUniforms(i);

  glActiveTexture(GL_TEXTURE0+i);
  glGenTextures(1, &images[i].id);
  glBindTexture(GL_TEXTURE_2D,images[i].id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
}

void updateImages(int force) {
  for (int i = 0; i<4; i++) {
    if (images[i].new || force) {
      readImage(i);
      printf("%s\n",images[i].path);
      images[i].new = 0;
    }
  }
}

void updateParams(int force) {
  for (int i = 0; i<32; i++) {
    if (parameters[i].new || force) {
      glUniform1f(glGetUniformLocation(shader.id, "params")+i, parameters[i].value);
      parameters[i].new = 0;
    }
  }
}

void *readStdin() {
  while (1) {
    char * str;
    size_t l = 80;
    getline(&str,&l,stdin);
    char *n = strtok(str," ");
    if (n[0] == 'i') {
      int i = atoi(strtok(NULL," "));
      strncpy(images[i].path, strtok(NULL,"\n"),40);
      images[i].new = 1;
    }
    else if (n[0] == 'f') {
      strncpy(shader.fragment, strtok(NULL,"\n"),40);
      shader.new = 1;
    }
    else if (n[0] == 'p') {
      int i = atoi(strtok(NULL," "));
      parameters[i].value = atof(strtok (NULL,"\n"));
      parameters[i].new = 1;
    }
    else if (n[0] == 'q') {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }
}

void *watchShader() {
  while (1) {
    sleep(0.2);
    struct stat file_stat;
    stat(shader.fragment, &file_stat);
    if (file_stat.st_mtime > shader.mtime) shader.new = 1; 
  }
}

int main(int argc, char **argv) {

  createWindow();
  strncpy(shader.vertex,"shader.vert",40);
  strncpy(shader.fragment,"shader.frag",40);
  shader.new = 1;
  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  for (int i = 1; i<argc; i++) {
    strncpy(images[i-1].path, argv[i],40);
    images[i-1].new = 1;
  }
  
  pthread_t stdin_t;
  pthread_create(&stdin_t, NULL, readStdin, NULL);
  pthread_t frag_t;
  pthread_create(&frag_t, NULL, watchShader, NULL);

  while (!glfwWindowShouldClose(window)) {
    glUniform1f(glGetUniformLocation(shader.id, "time"),glfwGetTime());
    if (shader.new) {
      createShader();
      uniform2f("resolution", width,height); // important!!
      for (int i = 0; i<4; i++) { imageUniforms(i); }
      updateParams(1);
      shader.new = 0;
    }
    updateImages(0);
    updateParams(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  pthread_cancel(stdin_t);
  pthread_cancel(frag_t);
  glfwDestroyWindow(window);
  glfwTerminate();
 
  exit(EXIT_SUCCESS);
}
