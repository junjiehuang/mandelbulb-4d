#include "common_nuklear_includes.hpp"
#include "drawing2.hpp"
#include "draw_mandel_wnd.hpp"

void drawMandelWnd(
  struct nk_context* ctx,
  unsigned int* texture,
  AppState* appState
)
{
  int retVal = nk_begin(
    ctx, "Mandelbulb, pow 8",
    nk_rect(50, 50, 800, 600),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE
  );

  if (retVal == 1) {
    drawMandelbulb(ctx, texture[0], appState);
  }

  nk_end(ctx);
}
