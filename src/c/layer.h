#pragma once

#include <ovbase.h>

struct aviutl2_edit_handle;
struct aviutl2_edit_section;
struct ptk_anm2editor;

/**
 * @brief Parameters for export layer names processing
 *
 * These parameters are passed from IPC callback to the main thread
 * for showing popup menu and handling layer export operations.
 */
struct ptk_layer_export_params {
  char const *file_path_utf8;
  char const *names_utf8;  // NUL-separated list of layer names
  char const *values_utf8; // NUL-separated list of layer values
  size_t names_len;        // Total length including NULs
  size_t values_len;       // Total length including NULs
  int32_t selected_index;
};

/**
 * @brief Parameters for export FAView slider processing
 *
 * These parameters are passed from IPC callback to the main thread
 * for showing popup menu and handling FAView slider export operations.
 */
struct ptk_faview_slider_export_params {
  char const *file_path_utf8;
  char const *slider_name_utf8; // Slider path, e.g., "*ささらちゃん\【前髪透過】"
  char const *names_utf8;       // NUL-separated list of item names
  char const *values_utf8;      // NUL-separated list of item values
  size_t names_len;             // Total length including NULs
  size_t values_len;            // Total length including NULs
  int32_t selected_index;
};

/**
 * @brief Process export layer names and show popup menu
 *
 * @param hwnd Parent window handle for dialogs
 * @param hwnd_foreign Foreign process window embedded as child (for focus handling, can be NULL)
 * @param edit Edit handle for accessing objects
 * @param anm2editor PSDToolKit anm2 Editor instance (can be NULL)
 * @param params Layer export parameters
 */
void ptk_layer_export(void *hwnd,
                      void *hwnd_foreign,
                      struct aviutl2_edit_handle *edit,
                      struct ptk_anm2editor *anm2editor,
                      struct ptk_layer_export_params const *params);

/**
 * @brief Process export FAView slider and show popup menu
 *
 * @param hwnd Parent window handle for dialogs
 * @param hwnd_foreign Foreign process window embedded as child (for focus handling, can be NULL)
 * @param edit Edit handle for accessing objects
 * @param anm2editor PSDToolKit anm2 Editor instance (can be NULL)
 * @param params FAView slider export parameters
 */
void ptk_faview_slider_export(void *hwnd,
                              void *hwnd_foreign,
                              struct aviutl2_edit_handle *edit,
                              struct ptk_anm2editor *anm2editor,
                              struct ptk_faview_slider_export_params const *params);
