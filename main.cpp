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
  float cell_size_ = 15.0f;
  bool additive_ = true;
  bool simulating_ = true;
  bool pressing_ = false;
};

struct color_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

// mutable globals
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static int64_t g_prev_ns = 0;

// constants
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

static as::vec2 board_top_left_corner(
  mc_gol_board_t* board, const float cell_size) {
  return as::vec2(
    (static_cast<float>(screen_dimensions.x) * 0.5f)
      - (mc_gol_board_width(board) * cell_size) * 0.5f,
    (static_cast<float>(screen_dimensions.y) * 0.5f)
      - (mc_gol_board_height(board) * cell_size) * 0.5f);
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

static void toggle_cell(
  game_of_life_t* game_of_life, const as::vec2& position) {
  const auto cell_size = game_of_life->cell_size_;
  const auto top_left = board_top_left_corner(game_of_life->board_, cell_size);
  const int32_t board_height = mc_gol_board_height(game_of_life->board_);
  const int32_t board_width = mc_gol_board_width(game_of_life->board_);
  for (int32_t y = 0; y < board_height; y++) {
    for (int32_t x = 0; x < board_width; x++) {
      const as::vec2 cell = top_left + as::vec2(x * cell_size, y * cell_size);
      if (
        position.x > cell.x && position.x <= cell.x + cell_size
        && position.y > cell.y && position.y <= cell.y + cell_size) {
        mc_gol_set_board_cell(
          game_of_life->board_, x, y, game_of_life->additive_);
      }
    }
  }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  SDL_SetAppMetadata("Game of Life", "1.0", "com.minimal-cmake.game-of-life");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer(
        "Game of Life", 800, 600, 0, &g_window, &g_renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_SetRenderVSync(g_renderer, 1); // enable vsync

  auto game_of_life = std::make_unique<game_of_life_t>();
  game_of_life->board_ = mc_gol_create_board(40, 27);
  reset_board(game_of_life->board_);
  *appstate = game_of_life.release();

  ImGui::CreateContext();
  ImGui::StyleColorsLight();

  ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
  ImGui_ImplSDLRenderer3_Init(g_renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  auto game_of_life = static_cast<game_of_life_t*>(appstate);

  const uint64_t now_ns = SDL_GetTicksNS();
  const uint64_t delta_ticks_ns = now_ns - g_prev_ns;
  g_prev_ns = now_ns;

  const double delta_time = delta_ticks_ns * 1.0e-9;
  game_of_life->timer_ += delta_time;

  const auto step_board = [game_of_life] {
    mc_gol_update_board(game_of_life->board_);
    game_of_life->timer_ = 0.0;
  };

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  SDL_SetRenderDrawColor(g_renderer, 242, 242, 242, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(g_renderer);

  if (ImGui::Begin("Game of Life")) {
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
    ImGui::Checkbox("Additive", &game_of_life->additive_);
  }
  ImGui::End();

  const auto cell_size = game_of_life->cell_size_;
  const auto top_left = board_top_left_corner(game_of_life->board_, cell_size);
  for (int32_t y = 0, height = mc_gol_board_height(game_of_life->board_);
       y < height; y++) {
    for (int32_t x = 0, width = mc_gol_board_width(game_of_life->board_);
         x < width; x++) {
      const as::vec2 cell_position =
        top_left + as::vec2(x * cell_size, y * cell_size);
      const color_t cell_color =
        mc_gol_board_cell(game_of_life->board_, x, y)
          ? color_t{.r = 242, .g = 181, .b = 105, .a = 255}
          : color_t{.r = 84, .g = 122, .b = 171, .a = 255};
      SDL_SetRenderDrawColor(
        g_renderer, cell_color.r, cell_color.g, cell_color.b, cell_color.a);
      const SDL_FRect cell = (SDL_FRect){.x = cell_position.x,
                                         .y = cell_position.y,
                                         .w = cell_size,
                                         .h = cell_size};
      SDL_RenderFillRect(g_renderer, &cell);
    }
  }

  SDL_SetRenderDrawColor(g_renderer, 39, 61, 113, 255);
  for (int32_t y = 0, height = mc_gol_board_height(game_of_life->board_);
       y <= height; y++) {
    SDL_RenderLine(
      g_renderer, top_left.x, top_left.y + y * cell_size,
      top_left.x + cell_size * mc_gol_board_width(game_of_life->board_),
      top_left.y + y * cell_size);
  }
  for (int32_t x = 0, width = mc_gol_board_width(game_of_life->board_);
       x <= width; x++) {
    SDL_RenderLine(
      g_renderer, top_left.x + x * cell_size, top_left.y,
      top_left.x + x * cell_size,
      top_left.y + cell_size * mc_gol_board_height(game_of_life->board_));
  }

  if (
    game_of_life->simulating_ && game_of_life->timer_ >= game_of_life->delay_) {
    step_board();
  }

  SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);

  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);

  SDL_RenderPresent(g_renderer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  ImGui_ImplSDL3_ProcessEvent(event);

  auto* game_of_life = static_cast<game_of_life_t*>(appstate);
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    SDL_MouseMotionEvent* mouse_motion = (SDL_MouseMotionEvent*)event;
    if (game_of_life->pressing_) {
      toggle_cell(game_of_life, as::vec2(mouse_motion->x, mouse_motion->y));
    }
  }

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      game_of_life->pressing_ = true;
      toggle_cell(game_of_life, as::vec2(mouse_button->x, mouse_button->y));
    }
  }

  if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    SDL_MouseButtonEvent* mouse_button = (SDL_MouseButtonEvent*)event;
    if (mouse_button->button == SDL_BUTTON_LEFT) {
      game_of_life->pressing_ = false;
    }
  }

  if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
    game_of_life->pressing_ = false;
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
