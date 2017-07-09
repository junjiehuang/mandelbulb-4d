/* nuklear - 1.32.0 - public domain */

#include <iostream>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

#include <GL/glew.h>
#include <glfw3.h>

#include <stdbool.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"
#include "drawing.cpp"
#include "drawing2.cpp"
#include "threads_test.cpp"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define UNUSED(a) (void)a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])


#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)


int test_opencl()
{
  cl_device_id device_id = NULL;
  cl_context context = NULL;
  cl_command_queue command_queue = NULL;
  cl_mem memobj = NULL;
  cl_program program = NULL;
  cl_kernel kernel = NULL;
  cl_platform_id platform_id = NULL;
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret;

  char string[MEM_SIZE];

  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    fprintf(stdout, "Current working dir: %s\n", cwd);
  } else {
    // perror("getcwd() error");
    fprintf(stderr,"getcwd() failed!\n");
    return 0;
  }

  FILE *fp;
  // char fileName[] = "C:/dev/cpp/hello.cl";
  char fileName[] = "hello.cl";

  #if defined(_WIN32) || defined(WIN32)
  char pathSeparator[] = "\\";
  #elif defined(__unix__) || defined(linux) || defined(__APPLE__)
  char pathSeparator[] = "/";
  #endif

  char * fullPath = (char*)malloc(sizeof(char) * (strlen(cwd) + strlen(pathSeparator) + strlen(fileName) + 1));
  if(fullPath != NULL) {
    fullPath[0] = '\0';   // ensures the memory is an empty string
    strcat(fullPath, cwd);
    strcat(fullPath, pathSeparator);
    strcat(fullPath, fileName);
  } else {
    fprintf(stderr,"malloc failed!\n");
    return 0;
  }

  char *source_str;
  size_t source_size;

  fprintf(stdout, "Full path: %s\n", fullPath);

  /* Load the source code containing the kernel*/
  fp = fopen(fullPath, "r");
  if (!fp) {
    fprintf(stderr, "Failed to load kernel.\n");
    exit(1);
  }
  source_str = (char*)malloc(MAX_SOURCE_SIZE);
  source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  /* Get Platform and Device Info */
  ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
  ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

  /* Create OpenCL context */
  context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

  /* Create Command Queue */
  command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

  /* Create Memory Buffer */
  memobj = clCreateBuffer(context, CL_MEM_READ_WRITE, MEM_SIZE * sizeof(char), NULL, &ret);

  /* Create Kernel Program from the source */
  program = clCreateProgramWithSource(
    context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret
  );

  /* Build Kernel Program */
  ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

  /* Create OpenCL Kernel */
  kernel = clCreateKernel(program, "hello", &ret);

  /* Set OpenCL Kernel Parameters */
  ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memobj);

  /* Execute OpenCL Kernel */
  ret = clEnqueueTask(command_queue, kernel, 0, NULL,NULL);

  /* Copy results from the memory buffer */
  ret = clEnqueueReadBuffer(
    command_queue, memobj, CL_TRUE, 0, MEM_SIZE * sizeof(char), string, 0, NULL, NULL
  );

  /* Display Result */
  puts(string);

  /* Finalization */
  ret = clFlush(command_queue);
  ret = clFinish(command_queue);
  ret = clReleaseKernel(kernel);
  ret = clReleaseProgram(program);
  ret = clReleaseMemObject(memobj);
  ret = clReleaseCommandQueue(command_queue);
  ret = clReleaseContext(context);

  free(source_str);

  return 0;
}


unsigned int mandel(double x0, double y0, double z0)
{
  const unsigned int n = 8;

  double nD = 0.0;

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  double r = 0.0;
  double theta = 0.0;
  double phi = 0.0;

  unsigned int i = 0;

  for (i = 0; i <= 32; i += 1) {
    nD = (double)n;

    r = sqrt(x * x + y * y + z * z);
    theta = atan2(sqrt(x * x + y * y), z);
    phi = atan2(y, x);

    x = pow(r, nD) * sin(theta * nD) * cos(phi * nD) + x0;
    y = pow(r, nD) * sin(theta * nD) * sin(phi * nD) + y0;
    z = pow(r, nD) * cos(theta * nD)                 + z0;

    if (pow(x, 2.0) + pow(y, 2.0) + pow(z, 2.0) > 2.0) {
      return 256 - (i * 4);
    }
  }

  return 0;
}

const unsigned int wMandel = 400;
const unsigned int hMandel = 400;

unsigned char arrayMandel[wMandel * hMandel * 4];

void generateMandel(int layer) {
  unsigned int m = 0;
  unsigned int i = 0;

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  for (unsigned int yy = 0; yy < hMandel; ++yy) {
    for (unsigned int xx = 0; xx < wMandel; ++xx) {
      x = 4.0 * ((double)xx - ((double)wMandel / 2.0)) / (double)wMandel;
      y = 4.0 * ((double)yy - ((double)hMandel / 2.0)) / (double)hMandel;
      z = (float)layer / 50.0;

      m = mandel(x, y, z);

      arrayMandel[i] = m;
      arrayMandel[i + 1] = m;
      arrayMandel[i + 2] = m;
      arrayMandel[i + 3] = 255;

      i += 4;
    }
  }

}

uint64_t mcg64(void)
{
    static uint64_t i = 1;
    return (i = (164603309694725029ull * i) % 14738995463583502973ull);
}

/*
bool is_power_of_2(uint64_t x) {
  return x == (x & -x);
}
*/

bool isPowerOfTwo(uint64_t x)
{
  return ((x != 0) && ((x & (~x + 1)) == x));
}

uint64_t urand64(void) {
  uint64_t hi = lrand48();
  uint64_t md = lrand48();
  uint64_t lo = lrand48();
  return (hi << 42) + (md << 21) + lo;
}

uint64_t unsigned_uniform_random(uint64_t low, uint64_t high) {
  static const uint64_t M = ~(uint64_t)0;
  uint64_t range = high - low;
  uint64_t to_exclude = isPowerOfTwo(range) ? 0
                                             : M % range + 1;
  uint64_t res;
  // Eliminate `to_exclude` possible values from consideration.
  while ((res = urand64()) < to_exclude) {}
  return low + res % range;
}

unsigned char *block = NULL;

void test64bit()
{
  const int64_t ARRAY_SIZE_6GB = 1024LL * 1024LL * 1024LL * 6;

  int64_t numLoops = 0;
  int64_t counter = 0;

  srand48(time(NULL));

  int64_t i1 = 0x0000444400004443LL;
  int64_t i2 = -100;

  block = (unsigned char *)calloc(sizeof(unsigned char) * ARRAY_SIZE_6GB, sizeof(unsigned char));

  if (block)
    std::cout << "Allocated 6 Gig block\n";
  else {
    std::cout << "Unable to allocate 6 Gig block.\n";

    exit(1);
  }

  block[235LL] = 235;

  numLoops = unsigned_uniform_random(3000, 3100);

  for (counter = 0; counter < numLoops; counter += 1) {
    i2 = -100;

    while (i2 <= 0 || i2 > ARRAY_SIZE_6GB) {
      i1 = unsigned_uniform_random(1, ARRAY_SIZE_6GB - 1);
      i2 = 1024LL * 1024LL * 1024LL * 6 - i1;
    }

    block[i2] = (unsigned char)unsigned_uniform_random(0, 255);
  }
}



/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the include
 * and the corresponding function. */
/*#include "../style.c"*/
/*#include "../calculator.c"*/
/*#include "../overview.c"*/
/*#include "../node_editor.c"*/

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

int main(void)
{
    test64bit();
    runThreads();
    test_opencl();
    generateMandel(21);

    /* Platform */
    static GLFWwindow *win;
    int width = 0, height = 0;
    struct nk_context *ctx;
    struct nk_color background;

    /* GLFW */
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Demo", NULL, NULL);
    glfwMakeContextCurrent(win);
    glfwGetWindowSize(win, &width, &height);

    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to setup GLEW\n");
        exit(1);
    }

    ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_glfw3_font_stash_end();
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &droid->handle);*/}

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/

    bool show_text = false;

    background = nk_rgb(28,48,62);
    while (!glfwWindowShouldClose(win))
    {
        /* Input */
        glfwPollEvents();
        nk_glfw3_new_frame();

        /* GUI */
        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                fprintf(stdout, "button pressed\n");

            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, background, nk_vec2(nk_widget_width(ctx),400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                background = nk_color_picker(ctx, background, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                background.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, background.r, 255, 1,1);
                background.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, background.g, 255, 1,1);
                background.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, background.b, 255, 1,1);
                background.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, background.a, 255, 1,1);
                nk_combo_end(ctx);
            }
        }
        nk_end(ctx);

        // creating a texture
        unsigned int texture;
        glGenTextures(1, &texture);

        if (nk_begin(ctx, "Nuklear", nk_rect(WINDOW_WIDTH/2 - 110 - 300, WINDOW_HEIGHT/2 - 110 - 50, 230, 230),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE))
        {
            draw_line(ctx, texture);

            // Widgets code here
            nk_layout_row_dynamic(ctx, 20, 1);

            if (show_text == true) {
                nk_label(ctx, "Hello, world!", NK_TEXT_LEFT);
            } else {
                nk_label(ctx, "", NK_TEXT_LEFT);
            }

            if (nk_button_label(ctx, "Clear")) {
                show_text = false;
            }

            if (nk_button_label(ctx, "Hello, world!")) {
                show_text = true;
            }
        }
        nk_end(ctx);

        // creating a texture
        unsigned int texture2;
        glGenTextures(1, &texture2);

        if (nk_begin(ctx, "Mandelbulb", nk_rect(WINDOW_WIDTH/2 - 110, WINDOW_HEIGHT/2 - 110, 420, 420),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE))
        {
            // draw_line(ctx, texture);
            draw_mandelbulb(ctx, texture2, arrayMandel, wMandel, hMandel);

            nk_layout_row_dynamic(ctx, 20, 1);
        }
        nk_end(ctx);

        /* -------------- EXAMPLES ---------------- */
        /*calculator(ctx);*/
        /*overview(ctx);*/
        /*node_editor(ctx);*/
        /* ----------------------------------------- */

        /* Draw */
        {
          float bg[4];
          nk_color_fv(bg, background);
          glfwGetWindowSize(win, &width, &height);
          glViewport(0, 0, width, height);
          glClear(GL_COLOR_BUFFER_BIT);
          glClearColor(bg[0], bg[1], bg[2], bg[3]);
          /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
           * with blending, scissor, face culling, depth test and viewport and
           * defaults everything back into a default state.
           * Make sure to either a.) save and restore or b.) reset your own state after
           * rendering the UI.
           */
          nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
          glfwSwapBuffers(win);
        }

        // delete texture
        glDeleteTextures(1, &texture);

        // delete texture
        glDeleteTextures(1, &texture2);
    }
    nk_glfw3_shutdown();
    glfwTerminate();

    free(block);

    return 0;
}
