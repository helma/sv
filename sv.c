#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define STB_IMAGE_IMPLEMENTATION
#include "std/stb_image.h"
#include "uthash/src/utstring.h"
#include "uthash/src/uthash.h"

int width = 1920;
int height = 1080;
GLFWwindow* window;
GLuint vertex;
GLuint fragment;
GLuint shader;
GLuint textures[4];

struct param {
  char name[10];
  float value;
  UT_hash_handle hh;         /* makes this structure hashable */
};

struct param *parameters  = NULL;

static void error_callback(int error, const char* description) { fputs(description, stderr); }

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
}

const char * readShader(char * path) {
  fputs(path,stderr);
  fputs("\n",stderr);
  FILE *file = fopen(path, "r");
  if (!file) {
    fputs("Cannot read ",stderr);
    fputs(path,stderr);
    exit(EXIT_FAILURE);
  }
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
    fputs("Shader compilation failed:\n",stderr);
    fputs(infoLog,stderr);
    exit(EXIT_FAILURE);
  }
  else { return sh; }
}

GLuint linkShader(GLuint vertex, GLuint fragment) {
  shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  int success;
  char infoLog[512];
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shader, 512, NULL, infoLog);
      fputs("Shader linking failed:\n",stderr);
      fputs(infoLog,stderr);
  }
  glDetachShader(shader, vertex); 
  glDetachShader(shader, fragment); 
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  glUseProgram(shader);
}

void createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  window = glfwCreateWindow(width,height, "", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetErrorCallback(error_callback);
  glewInit();
}

void createShader(char *vert, char *frag) {
  vertex = compileShader(readShader(vert), GL_VERTEX_SHADER);
  fragment = compileShader(readShader(frag),GL_FRAGMENT_SHADER);
  linkShader(vertex,fragment);
  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
};


void setTextureParams() {
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glEnable(GL_TEXTURE_2D);
  stbi_set_flip_vertically_on_load(1);
}
void readImage(char *file, char *name, int32_t t, int i) {

  GLuint tex;
  glGenTextures(1, &tex);
  glActiveTexture(GL_TEXTURE0+i);
  glBindTexture(GL_TEXTURE_2D,tex);
  setTextureParams();
  GLint v = glGetUniformLocation(shader, name);
  printf("loc %.i\n",v);
  glUniform1i(v, i);
  int comp;
  unsigned char* pixels = stbi_load(file, &width, &height, &comp, STBI_rgb_alpha);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
}

void uniform1f(char * var, float f) {
  int v = glGetUniformLocation(shader, var);
  glUniform1f(v, f);
}

void uniform2f(char * var, float f0, float f1) {
  int v = glGetUniformLocation(shader, var);
  glUniform2f(v,f0,f1);
}

void *readStdin() {
  while (1) {

    char * str;
    size_t l = 80;
    getline(&str,&l,stdin);
    char *n = strtok(str," ");
    UT_string *name;
    utstring_new(name);
    utstring_printf(name, n);

    if (utstring_find(name,0,"img",3) == 0) {
      //readImage(n,strtok(NULL,"\n"));
    }
    else if (utstring_find(name,0,"frag",4) == 0) {
      createShader("shader.vert",strtok(NULL,"\n"));
    }
    else if (utstring_find(name,0,"quit",4) == 0) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else {
      struct param *p;
      p = (struct param*)malloc(sizeof(struct param));
      strncpy(p->name, n,10);
      p->value = atof(strtok (NULL,"\n"));
      HASH_ADD_STR( parameters, name, p );
    }
  }
}

int main(int argc, char **argv) {

  createWindow();
  createShader("shader.vert","shader.frag");
  glUseProgram(shader);
  /*
  */
  readImage(argv[1],"img0",GL_TEXTURE0,0);
  readImage(argv[2],"img1",GL_TEXTURE1,1);
  readImage(argv[3],"img2",GL_TEXTURE2,2);
  readImage(argv[4],"img3",GL_TEXTURE3,3);
/*
  glEnable(GL_TEXTURE_2D);
  //glBindTexture(GL_TEXTURE_2D,0);
  GLuint tex0;
    glGenTextures(1, &tex0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,tex0);
  setTextureParams();

  GLint v = glGetUniformLocation(shader, "img0");
  printf("loc %.i\n",v);
  glUniform1i(v, tex0);
  int comp;
  unsigned char* pixels = stbi_load(argv[1], &width, &height, &comp, STBI_rgb_alpha);
  //printf("%i\n",textures[0]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
  //glBindTexture(GL_TEXTURE_2D,0);


  glBindTexture(GL_TEXTURE_2D,0);
  glActiveTexture(GL_TEXTURE0);
  GLuint tex1;
    glGenTextures(1, &tex1);
  v = glGetUniformLocation(shader, "img1");
  printf("loc %.i\n",v);
  glUniform1i(v, tex1);
  //glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,tex1);
  //glBindTexture(GL_TEXTURE_2D,textures[1]);
  //glBindTexture(GL_TEXTURE_2D,1);
  pixels = stbi_load(argv[2], &width, &height, &comp, STBI_rgb_alpha);
  printf("%i\n",tex1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
  //glBindTexture(GL_TEXTURE_2D,0);
*/

  uniform2f("resolution", width,height);
  pthread_t tid;
  pthread_create(&tid, NULL, readStdin, NULL);

  while (!glfwWindowShouldClose(window)) {
    uniform1f("time",glfwGetTime());
    // parameters mutex??
    // see uthash/doc/userguide.txt
    struct param *p, *tmp;
    HASH_ITER(hh, parameters, p, tmp) {
      printf("%s %.f\n",p->name,p->value);
      uniform1f(p->name,p->value);
      HASH_DEL(parameters, p);
      free(p);
    }
    /*
    struct texture *t, *tmp2;
    HASH_ITER(hh, textures, t, tmp2) {
      int v = glGetUniformLocation(shader, t->name);
      printf("loc %.i\n",v);
      glUniform1i(v, t->id);
    }
    */

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  //pthread_join(tid, NULL);
  //printf("After Thread\n");
  glfwDestroyWindow(window);
  glfwTerminate();
 
  exit(EXIT_SUCCESS);
}
