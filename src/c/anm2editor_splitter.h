#pragma once

#include <ovbase.h>

struct anm2editor_splitter;

/**
 * @brief Layout information calculated by the splitter
 */
struct anm2editor_splitter_layout {
  int left_x;      // X position for left pane
  int left_width;  // Width of left pane
  int right_x;     // X position for right pane
  int right_width; // Width of right pane
  int y;           // Y position for both panes
  int height;      // Height for both panes
};

/**
 * @brief Callbacks for splitter events
 */
struct anm2editor_splitter_callbacks {
  void *userdata;

  /**
   * @brief Called when splitter position changes (during drag or programmatically)
   *
   * @param userdata User data pointer
   */
  void (*on_position_changed)(void *userdata);
};

/**
 * @brief Create a splitter component
 *
 * @param width Width of the splitter bar in pixels
 * @param min_pane_width Minimum width for each pane
 * @param callbacks Event callbacks (can be NULL)
 * @param err Error information on failure
 * @return Created splitter, or NULL on failure
 */
NODISCARD struct anm2editor_splitter *anm2editor_splitter_create(int width,
                                                                 int min_pane_width,
                                                                 struct anm2editor_splitter_callbacks const *callbacks,
                                                                 struct ov_error *err);

/**
 * @brief Destroy a splitter component
 *
 * @param splitter Pointer to splitter to destroy, will be set to NULL
 */
void anm2editor_splitter_destroy(struct anm2editor_splitter **splitter);

/**
 * @brief Calculate layout for left and right panes
 *
 * @param splitter Splitter instance
 * @param content_x X position of content area
 * @param content_y Y position of content area
 * @param content_width Width of content area
 * @param content_height Height of content area
 * @param out Output layout information
 */
void anm2editor_splitter_calculate_layout(struct anm2editor_splitter *splitter,
                                          int content_x,
                                          int content_y,
                                          int content_width,
                                          int content_height,
                                          struct anm2editor_splitter_layout *out);

/**
 * @brief Get the current splitter position
 *
 * @param splitter Splitter instance
 * @return X position of the splitter (left edge)
 */
int anm2editor_splitter_get_position(struct anm2editor_splitter const *splitter);

/**
 * @brief Set the splitter position
 *
 * @param splitter Splitter instance
 * @param pos X position of the splitter (left edge)
 */
void anm2editor_splitter_set_position(struct anm2editor_splitter *splitter, int pos);

/**
 * @brief Initialize splitter position as a percentage of content width
 *
 * Only sets position if not already initialized (position < 0).
 *
 * @param splitter Splitter instance
 * @param content_width Total content width
 * @param percent Percentage (0-100) for left pane
 */
void anm2editor_splitter_init_position_percent(struct anm2editor_splitter *splitter, int content_width, int percent);

/**
 * @brief Get the splitter bar width
 *
 * @param splitter Splitter instance
 * @return Width of the splitter bar in pixels
 */
int anm2editor_splitter_get_width(struct anm2editor_splitter const *splitter);

/**
 * @brief Handle WM_SETCURSOR message
 *
 * Sets the resize cursor when over the splitter area.
 * Note: content_y is obtained from the last calculate_layout call.
 *
 * @param splitter Splitter instance
 * @param parent_window Parent window handle (HWND cast to void*)
 * @param cursor_x Cursor X position in client coordinates
 * @param cursor_y Cursor Y position in client coordinates
 * @return true if cursor was set (message handled), false otherwise
 */
bool anm2editor_splitter_handle_setcursor(struct anm2editor_splitter *splitter,
                                          void *parent_window,
                                          int cursor_x,
                                          int cursor_y);

/**
 * @brief Handle WM_LBUTTONDOWN message
 *
 * Starts drag operation if click is on the splitter.
 * Note: content_y is obtained from the last calculate_layout call.
 *
 * @param splitter Splitter instance
 * @param parent_window Parent window handle (HWND cast to void*)
 * @param x X position in client coordinates
 * @param y Y position in client coordinates
 * @return true if drag was started (message handled), false otherwise
 */
bool anm2editor_splitter_handle_lbutton_down(struct anm2editor_splitter *splitter, void *parent_window, int x, int y);

/**
 * @brief Handle WM_MOUSEMOVE message during drag
 *
 * Updates splitter position during drag operation.
 * Calls on_position_changed callback when position changes.
 *
 * @param splitter Splitter instance
 * @param parent_window Parent window handle (HWND cast to void*)
 * @param x Cursor X position in client coordinates
 * @return true if dragging (message handled), false otherwise
 */
bool anm2editor_splitter_handle_mouse_move(struct anm2editor_splitter *splitter, void *parent_window, int x);

/**
 * @brief Handle WM_LBUTTONUP message
 *
 * Ends drag operation.
 *
 * @param splitter Splitter instance
 * @param parent_window Parent window handle (HWND cast to void*)
 * @return true if drag was ended (message handled), false otherwise
 */
bool anm2editor_splitter_handle_lbutton_up(struct anm2editor_splitter *splitter, void *parent_window);
