#include "imgui_support.h"
#include "imgui/imgui.h"

#include "app.h"
#include "gfx.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

static Texture *imgui_texture;

bool imgui_wants_mouse(void) {
  return ImGui::GetIO().WantCaptureMouse;
}

bool imgui_wants_keyboard(void) {
  return ImGui::GetIO().WantCaptureKeyboard;
}

bool imgui_begin_simple_floating_window(const char *title, int x, int y, int w, int h) {
  ImGui::SetNextWindowPos(ImVec2((float)x, (float)y), 0, ImVec2(0,0));
  ImGui::SetNextWindowSizeConstraints(ImVec2((float)w, (float)h), ImVec2((float)w, (float)h), NULL, NULL);
  return ImGui::Begin(title, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
}

bool imgui_begin_simple_side_window(const char *title, bool leave_space_for_menu_bar) {
  float padding = 20;
  ImVec2 pos = ImVec2(padding, padding);
  if (leave_space_for_menu_bar)
    pos.y += 20;

  ImGui::SetNextWindowPos(pos, 0, ImVec2(0,0));
  ImGui::SetNextWindowSizeConstraints(ImVec2(-1, 50), ImVec2(-1, (float)gfx_height - padding), NULL, NULL);
  return ImGui::Begin(title, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
}



void setup_imgui(void) {
  setup_imgui_with_config(NULL);
}

static const char *imgui_get_clipboard(ImGuiContext* ctx) {
  return app_get_clipboard();
}
static void imgui_set_clipboard(ImGuiContext* ctx, const char *text) {
  app_set_clipboard(text);
}

void setup_imgui_with_config(Imgui_Config *conf) {
  static bool is_setup = false;
  if (is_setup)
    return;

  is_setup = true;

  ImGui::CreateContext(NULL);
  ImGuiIO *io = &ImGui::GetIO();
  io->FontGlobalScale = floorf(app_get_dpi_scale());

  ImGuiPlatformIO *pio = &ImGui::GetPlatformIO();
  //pio->Platform_GetClipboardTextFn = imgui_get_clipboard;
  //pio->Platform_SetClipboardTextFn = imgui_set_clipboard;

  if (conf && conf->ttf_font_path) {
    float size = conf->ttf_font_size;
    if (size <= 0)
      size = 16;
    io->Fonts->AddFontFromFileTTF(conf->ttf_font_path, size, NULL, NULL);
  }

  {
    unsigned char* pixels = NULL;
    int width = 0, height = 0;
    io->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, NULL);
    imgui_texture = gfx_new_texture(pixels, width, height);
    io->Fonts->SetTexID((size_t)imgui_texture);
    //gfx_set_filter_mode(imgui_texture, FILTER_MODE_LINEAR, FILTER_MODE_LINEAR);
  }

  ImGuiStyle *style = &ImGui::GetStyle();
  ImGui::StyleColorsDark(style);
  // style->WindowBorderSize = 0.0f;
  // style->WindowRounding = 0;

  ImVec4 dim_bg_color = {0.f, 0.f, 0.f, 0.5f};
  style->Colors[ImGuiCol_ModalWindowDimBg] = dim_bg_color;
  style->Colors[ImGuiCol_WindowBg].w = 0.3f;
  if (conf && conf->use_bg_alpha)
    style->Colors[ImGuiCol_WindowBg].w = conf->bg_alpha;
}

static int translate_to_imgui_key(int key) {
  if (key >= KEY_A && key <= KEY_Z) {
    return ImGuiKey_A + (key - KEY_A);
  }
  if (key >= KEY_0 && key <= KEY_9) {
    return ImGuiKey_0 + (key - KEY_0);
  }
  switch (key) {
  case KEY_MOUSE_LEFT:   return ImGuiMouseButton_Left;
  case KEY_MOUSE_RIGHT:  return ImGuiMouseButton_Right;
  case KEY_MOUSE_MIDDLE: return ImGuiMouseButton_Middle;
  case KEY_UP:    return ImGuiKey_UpArrow;
  case KEY_DOWN:  return ImGuiKey_DownArrow;
  case KEY_LEFT:  return ImGuiKey_LeftArrow;
  case KEY_RIGHT: return ImGuiKey_RightArrow;
  case KEY_SPACE: return ImGuiKey_Space;
  case KEY_ESCAPE: return ImGuiKey_Escape;
  case KEY_RETURN: return ImGuiKey_Enter;
  case KEY_DELETE: return ImGuiKey_Delete;
  case KEY_BACKSPACE: return ImGuiKey_Backspace;
  case KEY_TAB: return ImGuiKey_Tab;
  case KEY_LCTRL: return ImGuiKey_LeftCtrl;
  case KEY_LALT: return ImGuiKey_LeftAlt;
  case KEY_LSHIFT: return ImGuiKey_LeftShift;
  }
  return ImGuiKey_None;
}

void enter_imgui_frame(void) {
  setup_imgui();

  ImGuiIO *io = &ImGui::GetIO();
  ImVec2 v2 = {(float)app_mouse_x(), (float)app_mouse_y()};

  io->MousePos = v2;
  io->DisplaySize.x = (float)app_get_width();
  io->DisplaySize.y = (float)app_get_height();
  io->DeltaTime = app_delta();
  //io->AddKeyEvent(ImGuiKey_LeftCtrl, true)
  //io->KeyCtrl = app_keydown(KEY_LCTRL);
  //io->KeyAlt = app_keydown(KEY_LALT);
  //io->KeyShift = app_keydown(KEY_LSHIFT);
  for (int i = 0, c = 0; (c = app_get_char_input(i)); i++) {
    io->AddInputCharacter(c);
  }

  io->AddMousePosEvent((float)app_mouse_x(), (float)app_mouse_y());
  for (unsigned key = KEY_UNKNOWN+1; key < KEY_MAX_; key++) {
    int imgui_key = translate_to_imgui_key(key);
    if (!imgui_key)
      continue;

    if (key >= KEY_MOUSE_MIN_ && key <= KEY_MOUSE_MAX_)
      continue;

    ImGuiKey mod = ImGuiKey_None;
    if (key == KEY_LCTRL)
      mod = ImGuiKey_ModCtrl;
    if (key == KEY_LSHIFT)
      mod = ImGuiKey_ModShift;
    if (key == KEY_LALT)
      mod = ImGuiKey_ModAlt;

    if (app_keypress(key)) {
      io->AddKeyEvent((ImGuiKey)imgui_key, true);
      if (mod)
        io->AddKeyEvent(mod, true);
    }
    if (app_keyrelease(key)) {
      io->AddKeyEvent((ImGuiKey)imgui_key, false);
      if (mod)
        io->AddKeyEvent(mod, false);
    }
  }
  for (unsigned key = KEY_MOUSE_MIN_; key <= KEY_MOUSE_MAX_; key++) {
    int imgui_button = translate_to_imgui_key(key);
    if (app_keypress(key))
      io->AddMouseButtonEvent(imgui_button, true);
    if (app_keyrelease(key))
      io->AddMouseButtonEvent(imgui_button, false);
  }
  io->MouseWheel = app_mouse_wheel_y();

  ImGui::NewFrame();
}

void draw_imgui(void) {
  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();

	if (draw_data && draw_data->Valid) {
    for (int list_i = 0; list_i < draw_data->CmdListsCount; list_i++) {
      unsigned idx_count = 0;
      ImDrawList *draw_list = draw_data->CmdLists[list_i];
      for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++) {
        ImDrawCmd *cmd = &draw_list->CmdBuffer.Data[cmd_i];

        float clip_rect[4] = {cmd->ClipRect.x,cmd->ClipRect.y,cmd->ClipRect.z,cmd->ClipRect.w,};
        gfx_set_scissor(clip_rect[0], clip_rect[1], clip_rect[2]-clip_rect[0], clip_rect[3]-clip_rect[1]);

        gfx_imm_begin_2d();
        for (int idx_i = 0; idx_i < (int)cmd->ElemCount; idx_i++) {
          ImDrawIdx idx = draw_list->IdxBuffer.Data[idx_count + idx_i];
          const ImDrawVert *vtx = &draw_list->VtxBuffer.Data[idx];
          gfx_imm_vertex_2d(vtx->pos.x, vtx->pos.y, vtx->uv.x, vtx->uv.y, vtx->col);
        }
        gfx_imm_end();
        Texture *t = NULL;
        if (cmd->TextureId != NULL)
          t = (Texture *)cmd->TextureId;
        gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, t);

        idx_count += cmd->ElemCount;
      }
    }
  }
  gfx_unset_scissor();
}

bool imgui_any_hovered(void) {
  return ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

#ifdef __cplusplus
} // extern "C"
#endif

