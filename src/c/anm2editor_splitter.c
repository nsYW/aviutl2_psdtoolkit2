#include "anm2editor_splitter.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * @brief Internal structure for splitter component
 */
struct anm2editor_splitter {
  int width;                                      // Width of the splitter bar in pixels
  int min_pane_width;                             // Minimum width for each pane
  int position;                                   // Current X position (-1 = not initialized)
  int content_y;                                  // Y position where content area starts
  bool dragging;                                  // True if currently dragging
  struct anm2editor_splitter_callbacks callbacks; // Event callbacks
};

struct anm2editor_splitter *anm2editor_splitter_create(int const width,
                                                       int const min_pane_width,
                                                       struct anm2editor_splitter_callbacks const *const callbacks,
                                                       struct ov_error *const err) {
  struct anm2editor_splitter *splitter = NULL;
  bool success = false;

  if (!OV_REALLOC(&splitter, 1, sizeof(struct anm2editor_splitter))) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  *splitter = (struct anm2editor_splitter){
      .width = width,
      .min_pane_width = min_pane_width,
      .position = -1, // Not initialized
      .dragging = false,
      .callbacks = callbacks ? *callbacks : (struct anm2editor_splitter_callbacks){0},
  };

  success = true;

cleanup:
  if (!success) {
    if (splitter) {
      OV_FREE(&splitter);
    }
  }
  return splitter;
}

void anm2editor_splitter_destroy(struct anm2editor_splitter **const splitter) {
  if (!splitter || !*splitter) {
    return;
  }
  OV_FREE(splitter);
}

void anm2editor_splitter_calculate_layout(struct anm2editor_splitter *const splitter,
                                          int const content_x,
                                          int const content_y,
                                          int const content_width,
                                          int const content_height,
                                          struct anm2editor_splitter_layout *const out) {
  if (!splitter || !out) {
    return;
  }

  // Save content_y for mouse event handling
  splitter->content_y = content_y;

  // Initialize position if not set (use percentage from create)
  if (splitter->position < 0) {
    splitter->position = content_width * 40 / 100; // Default 40%
  }

  // Clamp position to valid range
  int pos = splitter->position;
  if (pos < splitter->min_pane_width) {
    pos = splitter->min_pane_width;
  }
  int const max_pos = content_width - splitter->min_pane_width - splitter->width;
  if (pos > max_pos) {
    pos = max_pos;
  }
  splitter->position = pos;

  // Calculate layout
  out->left_x = content_x;
  out->left_width = pos;
  out->right_x = content_x + pos + splitter->width;
  out->right_width = content_width - pos - splitter->width;
  out->y = content_y;
  out->height = content_height;
}

int anm2editor_splitter_get_position(struct anm2editor_splitter const *const splitter) {
  if (!splitter) {
    return 0;
  }
  return splitter->position;
}

void anm2editor_splitter_set_position(struct anm2editor_splitter *const splitter, int const pos) {
  if (!splitter) {
    return;
  }
  splitter->position = pos;
}

void anm2editor_splitter_init_position_percent(struct anm2editor_splitter *const splitter,
                                               int const content_width,
                                               int const percent) {
  if (!splitter) {
    return;
  }
  if (splitter->position < 0) {
    splitter->position = content_width * percent / 100;
  }
}

int anm2editor_splitter_get_width(struct anm2editor_splitter const *const splitter) {
  if (!splitter) {
    return 0;
  }
  return splitter->width;
}

// IDC_SIZEWE = 32644 (horizontal resize cursor)
static HCURSOR get_resize_cursor(void) { return LoadCursorW(NULL, MAKEINTRESOURCEW(32644)); }

static bool is_over_splitter(struct anm2editor_splitter const *const splitter, int const cursor_x, int const cursor_y) {
  if (!splitter || splitter->position < 0) {
    return false;
  }
  if (cursor_y < splitter->content_y) {
    return false;
  }
  return cursor_x >= splitter->position && cursor_x < splitter->position + splitter->width;
}

bool anm2editor_splitter_handle_setcursor(struct anm2editor_splitter *const splitter,
                                          void *const parent_window,
                                          int const cursor_x,
                                          int const cursor_y) {
  (void)parent_window;
  if (!splitter) {
    return false;
  }
  if (is_over_splitter(splitter, cursor_x, cursor_y)) {
    SetCursor(get_resize_cursor());
    return true;
  }
  return false;
}

bool anm2editor_splitter_handle_lbutton_down(struct anm2editor_splitter *const splitter,
                                             void *const parent_window,
                                             int const x,
                                             int const y) {
  if (!splitter || !parent_window) {
    return false;
  }
  if (is_over_splitter(splitter, x, y)) {
    splitter->dragging = true;
    SetCapture((HWND)parent_window);
    SetCursor(get_resize_cursor());
    return true;
  }
  return false;
}

bool anm2editor_splitter_handle_mouse_move(struct anm2editor_splitter *const splitter,
                                           void *const parent_window,
                                           int const x) {
  if (!splitter || !splitter->dragging || !parent_window) {
    return false;
  }

  RECT rc;
  GetClientRect((HWND)parent_window, &rc);
  int const content_width = rc.right - rc.left;

  // Calculate new position with constraints
  int new_pos = x;
  if (new_pos < splitter->min_pane_width) {
    new_pos = splitter->min_pane_width;
  }
  int const max_pos = content_width - splitter->min_pane_width - splitter->width;
  if (new_pos > max_pos) {
    new_pos = max_pos;
  }

  if (new_pos != splitter->position) {
    splitter->position = new_pos;
    // Notify position change
    if (splitter->callbacks.on_position_changed) {
      splitter->callbacks.on_position_changed(splitter->callbacks.userdata);
    }
  }
  return true;
}

bool anm2editor_splitter_handle_lbutton_up(struct anm2editor_splitter *const splitter, void *const parent_window) {
  (void)parent_window;
  if (!splitter || !splitter->dragging) {
    return false;
  }
  splitter->dragging = false;
  ReleaseCapture();
  return true;
}
