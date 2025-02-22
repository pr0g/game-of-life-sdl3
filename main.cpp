#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

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
const int32_t g_cell_size = 15;

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

  SDL_SetRenderVSync(renderer, 1); // enable vsync

  auto game_of_life = std::make_unique<game_of_life_t>();
  mc_gol_board_t* board = mc_gol_create_board(40, 27);
  game_of_life->board_ = board;
  reset_board(board);
  *appstate = game_of_life.release();

  ImGui::CreateContext();
  ImGui::StyleColorsLight();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  const uint64_t now = SDL_GetTicksNS();
  const uint64_t delta = now - g_prev;
  g_prev = now;
  const double delta_time = delta * 1.0e-9;

  auto game_of_life = static_cast<game_of_life_t*>(appstate);

  const auto step_board = [game_of_life] {
    mc_gol_update_board(game_of_life->board_);
    game_of_life->timer_ = 0.0;
  };

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  SDL_SetRenderDrawColor(renderer, 242, 242, 242, SDL_ALPHA_OPAQUE);
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
          ? color_t{.r = 242, .g = 181, .b = 105, .a = 255}
          : color_t{.r = 84, .g = 122, .b = 171, .a = 255};
      SDL_SetRenderDrawColor(
        renderer, cell_color.r, cell_color.g, cell_color.b, cell_color.a);
      const SDL_FRect cell = (SDL_FRect){.x = cell_position.x,
                                         .y = cell_position.y,
                                         .w = g_cell_size,
                                         .h = g_cell_size};
      SDL_RenderFillRect(renderer, &cell);
    }
  }

  SDL_SetRenderDrawColor(renderer, 39, 61, 113, 255);
  for (int32_t y = 0, height = mc_gol_board_height(game_of_life->board_);
       y <= height; y++) {
    SDL_RenderLine(
      renderer, board_top_left_corner.x,
      board_top_left_corner.y + y * g_cell_size,
      board_top_left_corner.x
        + g_cell_size * mc_gol_board_width(game_of_life->board_),
      board_top_left_corner.y + y * g_cell_size);
  }

  for (int32_t x = 0, width = mc_gol_board_width(game_of_life->board_);
       x <= width; x++) {
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
          const as::vec2i cell_position =
            board_top_left_corner + as::vec2i(x * g_cell_size, y * g_cell_size);
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
