#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "fps.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

#include <cassert>
#include <memory>
#include <numeric>

#include <as/as-math-ops.hpp>
#include <imgui.h>
#include <minimal-cmake-gol/gol.h>

struct game_of_life_t {
  mc_gol_board_t* board_ = nullptr;
  double timer_ = 0.0;
  float delay_ = 0.1f;
  bool simulating_ = true;
  as::vec2i mouse_now_;
};

struct color_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

static int64_t g_prev = 0;
fps::Fps g_fps;
const int32_t g_cell_size = 15;

static char debug_string[32];

constexpr double seconds_per_frame = 1.0 / 60.0;
constexpr int64_t nanoseconds_per_frame =
  static_cast<int64_t>(seconds_per_frame * 1.0e9);

const as::vec2i screen_dimensions = as::vec2i{800, 600};

static void clear_board(mc_gol_board_t* board) {
  for (int32_t y = 0, board_height = mc_gol_board_height(board);
       y < board_height; y++) {
    for (int32_t x = 0, board_width = mc_gol_board_width(board);
         x < board_width; x++) {
      mc_gol_set_board_cell(board, x, y, false);
    }
  }
}

static void reset_board(mc_gol_board_t* board) {
  // gosper glider gun
  mc_gol_set_board_cell(board, 2, 5, true);
  mc_gol_set_board_cell(board, 2, 6, true);
  mc_gol_set_board_cell(board, 3, 5, true);
  mc_gol_set_board_cell(board, 3, 6, true);

  mc_gol_set_board_cell(board, 12, 5, true);
  mc_gol_set_board_cell(board, 12, 6, true);
  mc_gol_set_board_cell(board, 12, 7, true);
  mc_gol_set_board_cell(board, 13, 4, true);
  mc_gol_set_board_cell(board, 13, 8, true);
  mc_gol_set_board_cell(board, 14, 3, true);
  mc_gol_set_board_cell(board, 14, 9, true);
  mc_gol_set_board_cell(board, 15, 3, true);
  mc_gol_set_board_cell(board, 15, 9, true);
  mc_gol_set_board_cell(board, 16, 6, true);
  mc_gol_set_board_cell(board, 17, 4, true);
  mc_gol_set_board_cell(board, 17, 8, true);
  mc_gol_set_board_cell(board, 18, 5, true);
  mc_gol_set_board_cell(board, 18, 6, true);
  mc_gol_set_board_cell(board, 18, 7, true);
  mc_gol_set_board_cell(board, 19, 6, true);

  mc_gol_set_board_cell(board, 22, 3, true);
  mc_gol_set_board_cell(board, 22, 4, true);
  mc_gol_set_board_cell(board, 22, 5, true);
  mc_gol_set_board_cell(board, 23, 3, true);
  mc_gol_set_board_cell(board, 23, 4, true);
  mc_gol_set_board_cell(board, 23, 5, true);
  mc_gol_set_board_cell(board, 24, 2, true);
  mc_gol_set_board_cell(board, 24, 6, true);
  mc_gol_set_board_cell(board, 26, 1, true);
  mc_gol_set_board_cell(board, 26, 2, true);
  mc_gol_set_board_cell(board, 26, 6, true);
  mc_gol_set_board_cell(board, 26, 7, true);

  mc_gol_set_board_cell(board, 36, 3, true);
  mc_gol_set_board_cell(board, 36, 4, true);
  mc_gol_set_board_cell(board, 37, 3, true);
  mc_gol_set_board_cell(board, 37, 4, true);

  // eater
  mc_gol_set_board_cell(board, 27, 20, true);
  mc_gol_set_board_cell(board, 27, 21, true);
  mc_gol_set_board_cell(board, 28, 20, true);
  mc_gol_set_board_cell(board, 28, 21, true);

  mc_gol_set_board_cell(board, 32, 21, true);
  mc_gol_set_board_cell(board, 31, 22, true);
  mc_gol_set_board_cell(board, 33, 22, true);
  mc_gol_set_board_cell(board, 32, 23, true);

  mc_gol_set_board_cell(board, 34, 23, true);
  mc_gol_set_board_cell(board, 34, 24, true);
  mc_gol_set_board_cell(board, 34, 25, true);
  mc_gol_set_board_cell(board, 35, 25, true);
}

void wait_to_update(const int64_t now) {
  // reference: https://davidgow.net/handmadepenguin/ch18.html
  const int64_t frame_nanoseconds = now - g_prev;
  if (frame_nanoseconds < nanoseconds_per_frame) {
    const int64_t remainder_ns = nanoseconds_per_frame - frame_nanoseconds;
    const int64_t remainder_pad_ns = std::max(
      remainder_ns - static_cast<int64_t>(4 * SDL_NS_PER_MS),
      static_cast<int64_t>(0));
    SDL_DelayNS(static_cast<uint64_t>(remainder_pad_ns));
    const int64_t nanoseconds_since = SDL_GetTicksNS() - g_prev;
    const int64_t nanoseconds_left = nanoseconds_per_frame - nanoseconds_since;
    // if (nanoseconds_left < 0) {
    //   SDL_Log("nanonseconds left %" SDL_PRIs64, nanoseconds_left);
    // }
    // check we didn't wait too long and get behind
    // assert(nanoseconds_since < nanoseconds_per_frame);
    while (SDL_GetTicksNS() - g_prev < nanoseconds_per_frame) {
      ;
    }
  }
}

// int main(int argc, char** argv) {
//   (void)argc;
//   (void)argv;

//   if (!SDL_Init(SDL_INIT_VIDEO)) {
//     fprintf(
//       stderr, "SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
//     return 1;
//   }

//   SDL_Window* window = SDL_CreateWindow(argv[0], 800, 600, 0);

//   for (bool running = true; running;) {
//     for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
//       if (current_event.type == SDL_EVENT_MOUSE_MOTION) {
//         SDL_MouseMotionEvent* mouse_motion =
//           (SDL_MouseMotionEvent*)&current_event;
//         SDL_Log("mouse motion %f, %f", mouse_motion->x, mouse_motion->y);
//       }
//     }
//   }
// }

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  SDL_SetAppMetadata("Game of Life", "1.0", "com.minimal-cmake.game-of-life");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(
        "Game of Life", 800, 600, 0, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED);

  debug_string[0] = '\0';

  mc_gol_board_t* board = mc_gol_create_board(40, 27);

  reset_board(board);

  auto game_of_life = std::make_unique<game_of_life_t>();

  game_of_life->board_ = board;

  *appstate = game_of_life.release();

  ImGui::CreateContext();
  ImGui::StyleColorsLight();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  const uint64_t now = SDL_GetTicksNS();
  wait_to_update(now);
  const double now_fp = static_cast<double>(now) / 1.0e-9;
  const uint64_t delta = now - g_prev;
  g_prev = now;

  const double delta_time = delta * 1.0e-9;

  const int64_t window = fps::calculateWindow(g_fps, SDL_GetTicksNS());
  double framerate =
    window > -1 ? (double)(g_fps.MaxSamples - 1) / (double(window) * 1.0e-9)
                : 0.0;

  auto game_of_life = static_cast<game_of_life_t*>(appstate);

  SDL_snprintf(
    debug_string, sizeof(debug_string), "%d %d", game_of_life->mouse_now_.x,
    game_of_life->mouse_now_.y);

  const auto step_board = [game_of_life] {
    mc_gol_update_board(game_of_life->board_);
    game_of_life->timer_ = 0.0;
  };

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  SDL_SetRenderDrawColorFloat(
    renderer, 0.5f, 0.5f, 0.5f, SDL_ALPHA_OPAQUE_FLOAT);
  SDL_RenderClear(renderer);

  ImGui::Begin("Game of Life");
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat(
    "Simulation time", &game_of_life->delay_, 0.01f, 1.0f, "%.2f",
    ImGuiSliderFlags_AlwaysClamp);
  ImGui::PopItemWidth();
  if (ImGui::Button(game_of_life->simulating_ ? "Pause" : "Play")) {
    game_of_life->timer_ = 0.0;
    game_of_life->simulating_ = !game_of_life->simulating_;
  }
  if (game_of_life->simulating_) {
    ImGui::BeginDisabled();
  }
  ImGui::SameLine();
  if (ImGui::Button("Step")) {
    step_board();
  }
  if (game_of_life->simulating_) {
    ImGui::EndDisabled();
  }
  if (ImGui::Button("Clear")) {
    clear_board(game_of_life->board_);
    game_of_life->simulating_ = false;
  }
  if (ImGui::Button("Restart")) {
    clear_board(game_of_life->board_);
    reset_board(game_of_life->board_);
  }
  ImGui::End();

  const auto board_top_left_corner = as::vec2(
    (screen_dimensions.x / 2.0f)
      - (mc_gol_board_width(game_of_life->board_) * g_cell_size) * 0.5f,
    (screen_dimensions.y / 2.0f)
      - (mc_gol_board_height(game_of_life->board_) * g_cell_size) * 0.5f);
  for (int32_t y = 0, height = mc_gol_board_height(game_of_life->board_);
       y < height; y++) {
    for (int32_t x = 0, width = mc_gol_board_width(game_of_life->board_);
         x < width; x++) {
      const as::vec2 cell_position =
        board_top_left_corner + as::vec2(x * g_cell_size, y * g_cell_size);
      const color_t cell_color =
        mc_gol_board_cell(game_of_life->board_, x, y)
          ? color_t{.r = 255, .g = 255, .b = 255, .a = 255}
          : color_t{.a = 255};
      SDL_SetRenderDrawColor(
        renderer, cell_color.r, cell_color.g, cell_color.b, cell_color.a);
      const SDL_FRect cell = (SDL_FRect){.x = cell_position.x,
                                         .y = cell_position.y,
                                         .w = g_cell_size,
                                         .h = g_cell_size};
      SDL_RenderFillRect(renderer, &cell);
    }
  }

  for (int32_t y = 0, height = mc_gol_board_height(game_of_life->board_);
       y <= height; y++) {
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 1);
    SDL_RenderLine(
      renderer, board_top_left_corner.x,
      board_top_left_corner.y + y * g_cell_size,
      board_top_left_corner.x
        + g_cell_size * mc_gol_board_width(game_of_life->board_),
      board_top_left_corner.y + y * g_cell_size);
  }

  for (int32_t x = 0, width = mc_gol_board_width(game_of_life->board_);
       x <= width; x++) {
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 1);
    SDL_RenderLine(
      renderer, board_top_left_corner.x + x * g_cell_size,
      board_top_left_corner.y, board_top_left_corner.x + x * g_cell_size,
      board_top_left_corner.y
        + g_cell_size * mc_gol_board_height(game_of_life->board_));
  }

  game_of_life->timer_ += delta_time;
  if (
    game_of_life->simulating_ && game_of_life->timer_ >= game_of_life->delay_) {
    step_board();
  }

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(renderer, 5, 5, debug_string);

  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  ImGui_ImplSDL3_ProcessEvent(event);

  auto* game_of_life = static_cast<game_of_life_t*>(appstate);
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    SDL_MouseMotionEvent* mouse_motion = (SDL_MouseMotionEvent*)event;
    game_of_life->mouse_now_ = as::vec2i(mouse_motion->x, mouse_motion->y);
  }

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      for (int32_t y = 0,
                   board_height = mc_gol_board_height(game_of_life->board_);
           y < board_height; y++) {
        for (int32_t x = 0,
                     board_width = mc_gol_board_width(game_of_life->board_);
             x < board_width; x++) {
          const auto board_top_left_corner = as::vec2i(
            (screen_dimensions.x / 2.0f)
              - (mc_gol_board_width(game_of_life->board_) * g_cell_size) * 0.5f,
            (screen_dimensions.y / 2.0f)
              - (mc_gol_board_height(game_of_life->board_) * g_cell_size)
                  * 0.5f);
          const as::vec2i position = game_of_life->mouse_now_;
          SDL_Log("mouse motion %d, %d", position.x, position.y);
          const as::vec2i cell_position =
            board_top_left_corner + as::vec2i(x * g_cell_size, y * g_cell_size);
          SDL_Log("cell_position %d, %d", cell_position.x, cell_position.y);
          if (
            position.x > cell_position.x
            && position.x <= cell_position.x + g_cell_size
            && position.y > cell_position.y
            && position.y <= cell_position.y + g_cell_size) {
            mc_gol_set_board_cell(
              game_of_life->board_, x, y,
              !mc_gol_board_cell(game_of_life->board_, x, y));
          }
        }
      }
    }
  }

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  const auto game_of_life = static_cast<game_of_life_t*>(appstate);
  mc_gol_destroy_board(game_of_life->board_);
  delete game_of_life;

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}
