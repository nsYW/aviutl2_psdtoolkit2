#include "anm2editor_toolbar.h"

#include <ovmo.h>
#include <ovutf.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>
#include <objidl.h>

#include <gdiplus/gdiplus.h>

#include <ovl/os.h>

// Toolbar icon resource IDs (must match anm2editor.rc)
enum {
  IDB_TOOLBAR_IMPORT = 101,
  IDB_TOOLBAR_NEW = 102,
  IDB_TOOLBAR_OPEN = 103,
  IDB_TOOLBAR_SAVE = 104,
  IDB_TOOLBAR_SAVEAS = 105,
  IDB_TOOLBAR_UNDO = 106,
  IDB_TOOLBAR_REDO = 107,
  IDB_TOOLBAR_CONVERT = 108,
};

// Internal command IDs (not exposed to external code)
enum {
  CMD_FILE_NEW = 40001,
  CMD_FILE_OPEN = 40002,
  CMD_FILE_SAVE = 40003,
  CMD_FILE_SAVEAS = 40004,
  CMD_EDIT_UNDO = 40110,
  CMD_EDIT_REDO = 40111,
  CMD_EDIT_IMPORT_SCRIPTS = 40109,
  CMD_EDIT_CONVERT_ANM = 40112,
};

// Toolbar icon indices in ImageList
enum {
  ICON_NEW,
  ICON_OPEN,
  ICON_SAVE,
  ICON_SAVEAS,
  ICON_UNDO,
  ICON_REDO,
  ICON_IMPORT,
  ICON_CONVERT,
  ICON_COUNT,
};

enum {
  ICON_SIZE = 24,
  BUTTON_WIDTH = 40,
  BUTTON_HEIGHT = 28,
};

// Mapping from icon index to resource ID
static int const icon_resources[ICON_COUNT] = {
    [ICON_NEW] = IDB_TOOLBAR_NEW,
    [ICON_OPEN] = IDB_TOOLBAR_OPEN,
    [ICON_SAVE] = IDB_TOOLBAR_SAVE,
    [ICON_SAVEAS] = IDB_TOOLBAR_SAVEAS,
    [ICON_UNDO] = IDB_TOOLBAR_UNDO,
    [ICON_REDO] = IDB_TOOLBAR_REDO,
    [ICON_IMPORT] = IDB_TOOLBAR_IMPORT,
    [ICON_CONVERT] = IDB_TOOLBAR_CONVERT,
};

/**
 * @brief Internal structure for toolbar component
 */
struct anm2editor_toolbar {
  HWND hwnd;                                     // Toolbar window handle
  HIMAGELIST imagelist;                          // Normal icons
  HIMAGELIST disabled_imagelist;                 // Disabled (grayed) icons
  struct anm2editor_toolbar_callbacks callbacks; // Event callbacks
};

// Load a single PNG resource and add to both normal and disabled ImageLists.
// GDI+ must already be initialized.
static bool load_single_icon(HINSTANCE const hinst,
                             int const resource_id,
                             int const target_size,
                             HIMAGELIST const normal_list,
                             HIMAGELIST const disabled_list) {
  bool success = false;
  GpBitmap *src_bitmap = NULL;
  GpBitmap *scaled_bitmap = NULL;
  GpBitmap *disabled_bitmap = NULL;
  GpGraphics *graphics = NULL;
  GpImageAttributes *image_attr = NULL;
  IStream *stream = NULL;
  HGLOBAL hmem = NULL;
  void *mem_ptr = NULL;
  HRSRC hres = NULL;
  HGLOBAL hres_data = NULL;
  void const *res_ptr = NULL;
  DWORD res_size = 0;
  HBITMAP hbmp_normal = NULL;
  HBITMAP hbmp_disabled = NULL;
  // Grayscale matrix with 25% alpha
  // clang-format off
  ColorMatrix gray_matrix = {{
      {0.299f, 0.299f, 0.299f, 0.0f, 0.0f},
      {0.587f, 0.587f, 0.587f, 0.0f, 0.0f},
      {0.114f, 0.114f, 0.114f, 0.0f, 0.0f},
      {0.0f,   0.0f,   0.0f,   0.25f, 0.0f},
      {0.0f,   0.0f,   0.0f,   0.0f, 1.0f},
  }};
  // clang-format on

  // Find and load PNG resource
  hres = FindResourceW(hinst, MAKEINTRESOURCEW(resource_id), L"PNG");
  if (!hres) {
    goto cleanup;
  }

  res_size = SizeofResource(hinst, hres);
  hres_data = LoadResource(hinst, hres);
  if (!hres_data) {
    goto cleanup;
  }

  res_ptr = LockResource(hres_data);
  if (!res_ptr) {
    goto cleanup;
  }

  // Create IStream from resource data
  hmem = GlobalAlloc(GMEM_MOVEABLE, res_size);
  if (!hmem) {
    goto cleanup;
  }

  mem_ptr = GlobalLock(hmem);
  if (!mem_ptr) {
    goto cleanup;
  }
  memcpy(mem_ptr, res_ptr, res_size);
  GlobalUnlock(hmem);
  mem_ptr = NULL;

  if (FAILED(CreateStreamOnHGlobal(hmem, TRUE, &stream))) {
    goto cleanup;
  }
  hmem = NULL; // Stream now owns hmem

  // Create GDI+ bitmap from stream
  if (GdipCreateBitmapFromStream(stream, &src_bitmap) != Ok) {
    goto cleanup;
  }

  // Create scaled bitmap for normal icon
  if (GdipCreateBitmapFromScan0(target_size, target_size, 0, PixelFormat32bppPARGB, NULL, &scaled_bitmap) != Ok) {
    goto cleanup;
  }

  if (GdipGetImageGraphicsContext((GpImage *)scaled_bitmap, &graphics) != Ok) {
    goto cleanup;
  }

  GdipSetInterpolationMode(graphics, InterpolationModeHighQualityBicubic);
  GdipSetPixelOffsetMode(graphics, PixelOffsetModeHighSpeed);
  GdipDrawImageRectI(graphics, (GpImage *)src_bitmap, 0, 0, target_size, target_size);

  GdipDeleteGraphics(graphics);
  graphics = NULL;

  // Create HBITMAP for normal icon
  if (GdipCreateHBITMAPFromBitmap(scaled_bitmap, &hbmp_normal, 0x00000000) != Ok) {
    goto cleanup;
  }

  // Create disabled bitmap with grayscale + reduced alpha using ColorMatrix
  if (GdipCreateBitmapFromScan0(target_size, target_size, 0, PixelFormat32bppPARGB, NULL, &disabled_bitmap) != Ok) {
    goto cleanup;
  }

  if (GdipGetImageGraphicsContext((GpImage *)disabled_bitmap, &graphics) != Ok) {
    goto cleanup;
  }

  // Create ImageAttributes with grayscale + alpha reduction ColorMatrix
  if (GdipCreateImageAttributes(&image_attr) != Ok) {
    goto cleanup;
  }

  if (GdipSetImageAttributesColorMatrix(
          image_attr, ColorAdjustTypeDefault, TRUE, &gray_matrix, NULL, ColorMatrixFlagsDefault) != Ok) {
    goto cleanup;
  }

  // Draw scaled bitmap with grayscale effect (no interpolation needed, same size)
  GdipDrawImageRectRectI(graphics,
                         (GpImage *)scaled_bitmap,
                         0,
                         0,
                         target_size,
                         target_size,
                         0,
                         0,
                         target_size,
                         target_size,
                         UnitPixel,
                         image_attr,
                         NULL,
                         NULL);

  // Create HBITMAP for disabled icon
  if (GdipCreateHBITMAPFromBitmap(disabled_bitmap, &hbmp_disabled, 0x00000000) != Ok) {
    goto cleanup;
  }

  // Add to ImageLists
  ImageList_Add(normal_list, hbmp_normal, NULL);
  ImageList_Add(disabled_list, hbmp_disabled, NULL);

  success = true;

cleanup:
  if (hbmp_disabled) {
    DeleteObject(hbmp_disabled);
  }
  if (hbmp_normal) {
    DeleteObject(hbmp_normal);
  }
  if (image_attr) {
    GdipDisposeImageAttributes(image_attr);
  }
  if (graphics) {
    GdipDeleteGraphics(graphics);
  }
  if (disabled_bitmap) {
    GdipDisposeImage((GpImage *)disabled_bitmap);
  }
  if (scaled_bitmap) {
    GdipDisposeImage((GpImage *)scaled_bitmap);
  }
  if (src_bitmap) {
    GdipDisposeImage((GpImage *)src_bitmap);
  }
  if (stream) {
    stream->lpVtbl->Release(stream);
  }
  if (hmem) {
    GlobalFree(hmem);
  }
  return success;
}

// Load all toolbar icons into ImageLists.
static bool load_toolbar_icons(HINSTANCE const hinst, HIMAGELIST const normal_list, HIMAGELIST const disabled_list) {
  ULONG_PTR gdiplus_token = 0;
  GdiplusStartupInput startup_input = {.GdiplusVersion = 1};
  bool success = false;

  // Initialize GDI+ once for all icons
  if (GdiplusStartup(&gdiplus_token, &startup_input, NULL) != Ok) {
    return false;
  }

  // Load each icon
  for (int i = 0; i < ICON_COUNT; i++) {
    if (!load_single_icon(hinst, icon_resources[i], ICON_SIZE, normal_list, disabled_list)) {
      goto cleanup;
    }
  }

  success = true;

cleanup:
  if (gdiplus_token) {
    GdiplusShutdown(gdiplus_token);
  }
  return success;
}

struct anm2editor_toolbar *anm2editor_toolbar_create(void *const parent_window,
                                                     int const control_id,
                                                     struct anm2editor_toolbar_callbacks const *const callbacks,
                                                     struct ov_error *const err) {
  struct anm2editor_toolbar *toolbar = NULL;
  bool success = false;

  if (!parent_window) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    goto cleanup;
  }

  if (!OV_REALLOC(&toolbar, 1, sizeof(struct anm2editor_toolbar))) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  *toolbar = (struct anm2editor_toolbar){0};

  // Store callbacks
  if (callbacks) {
    toolbar->callbacks = *callbacks;
  }

  // Create Toolbar window
  toolbar->hwnd = CreateWindowExW(0,
                                  TOOLBARCLASSNAMEW,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP,
                                  0,
                                  0,
                                  0,
                                  0,
                                  (HWND)parent_window,
                                  (HMENU)(uintptr_t)control_id,
                                  GetModuleHandleW(NULL),
                                  NULL);
  if (!toolbar->hwnd) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  SendMessageW(toolbar->hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

  // Set toolbar icon and button size
  SendMessageW(toolbar->hwnd, TB_SETBITMAPSIZE, 0, MAKELPARAM(ICON_SIZE, ICON_SIZE));
  SendMessageW(toolbar->hwnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(BUTTON_WIDTH, BUTTON_HEIGHT));

  // Create custom ImageList for toolbar (normal and disabled)
  toolbar->imagelist = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32, ICON_COUNT, 4);
  toolbar->disabled_imagelist = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32, ICON_COUNT, 4);

  // Load PNG icons (initializes GDI+ once for all icons)
  {
    void *dll_hinst = NULL;
    ovl_os_get_hinstance_from_fnptr((void *)load_toolbar_icons, &dll_hinst, NULL);
    load_toolbar_icons((HINSTANCE)dll_hinst, toolbar->imagelist, toolbar->disabled_imagelist);
  }

  // Set the custom ImageList to toolbar (normal and disabled)
  SendMessageW(toolbar->hwnd, TB_SETIMAGELIST, 0, (LPARAM)toolbar->imagelist);
  SendMessageW(toolbar->hwnd, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)toolbar->disabled_imagelist);

  // Add toolbar buttons (using internal command IDs)
  {
    TBBUTTON buttons[] = {
        {ICON_NEW, CMD_FILE_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {ICON_OPEN, CMD_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {ICON_SAVE, CMD_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {ICON_SAVEAS, CMD_FILE_SAVEAS, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {0, 0, 0, BTNS_SEP, {0}, 0, 0},
        {ICON_UNDO, CMD_EDIT_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {ICON_REDO, CMD_EDIT_REDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {0, 0, 0, BTNS_SEP, {0}, 0, 0},
        {ICON_IMPORT, CMD_EDIT_IMPORT_SCRIPTS, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {ICON_CONVERT, CMD_EDIT_CONVERT_ANM, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    };
    SendMessageW(toolbar->hwnd, TB_ADDBUTTONSW, sizeof(buttons) / sizeof(buttons[0]), (LPARAM)buttons);
  }

  success = true;

cleanup:
  if (!success) {
    if (toolbar) {
      if (toolbar->imagelist) {
        ImageList_Destroy(toolbar->imagelist);
      }
      if (toolbar->disabled_imagelist) {
        ImageList_Destroy(toolbar->disabled_imagelist);
      }
      if (toolbar->hwnd) {
        DestroyWindow(toolbar->hwnd);
      }
      OV_FREE(&toolbar);
    }
  }
  return toolbar;
}

void anm2editor_toolbar_destroy(struct anm2editor_toolbar **const toolbar) {
  if (!toolbar || !*toolbar) {
    return;
  }
  struct anm2editor_toolbar *t = *toolbar;
  if (t->imagelist) {
    ImageList_Destroy(t->imagelist);
    t->imagelist = NULL;
  }
  if (t->disabled_imagelist) {
    ImageList_Destroy(t->disabled_imagelist);
    t->disabled_imagelist = NULL;
  }
  // Note: hwnd is destroyed by parent window
  OV_FREE(toolbar);
}

void *anm2editor_toolbar_get_window(struct anm2editor_toolbar *const toolbar) {
  if (!toolbar) {
    return NULL;
  }
  return toolbar->hwnd;
}

void anm2editor_toolbar_autosize(struct anm2editor_toolbar *const toolbar) {
  if (!toolbar || !toolbar->hwnd) {
    return;
  }
  SendMessageW(toolbar->hwnd, TB_AUTOSIZE, 0, 0);
}

int anm2editor_toolbar_get_height(struct anm2editor_toolbar *const toolbar) {
  if (!toolbar || !toolbar->hwnd) {
    return 0;
  }
  RECT rc;
  GetWindowRect(toolbar->hwnd, &rc);
  return rc.bottom - rc.top;
}

void anm2editor_toolbar_update_state(struct anm2editor_toolbar *const toolbar,
                                     bool const can_undo,
                                     bool const can_redo,
                                     bool const can_save) {
  if (!toolbar || !toolbar->hwnd) {
    return;
  }
  SendMessageW(toolbar->hwnd, TB_ENABLEBUTTON, CMD_EDIT_UNDO, MAKELPARAM(can_undo ? TRUE : FALSE, 0));
  SendMessageW(toolbar->hwnd, TB_ENABLEBUTTON, CMD_EDIT_REDO, MAKELPARAM(can_redo ? TRUE : FALSE, 0));
  SendMessageW(toolbar->hwnd, TB_ENABLEBUTTON, CMD_FILE_SAVE, MAKELPARAM(can_save ? TRUE : FALSE, 0));
  SendMessageW(toolbar->hwnd, TB_ENABLEBUTTON, CMD_FILE_SAVEAS, MAKELPARAM(can_save ? TRUE : FALSE, 0));
}

bool anm2editor_toolbar_handle_command(struct anm2editor_toolbar *const toolbar, int const cmd_id) {
  if (!toolbar) {
    return false;
  }

  void *const userdata = toolbar->callbacks.userdata;

  switch (cmd_id) {
  case CMD_FILE_NEW:
    if (toolbar->callbacks.on_file_new) {
      toolbar->callbacks.on_file_new(userdata);
    }
    return true;

  case CMD_FILE_OPEN:
    if (toolbar->callbacks.on_file_open) {
      toolbar->callbacks.on_file_open(userdata);
    }
    return true;

  case CMD_FILE_SAVE:
    if (toolbar->callbacks.on_file_save) {
      toolbar->callbacks.on_file_save(userdata);
    }
    return true;

  case CMD_FILE_SAVEAS:
    if (toolbar->callbacks.on_file_saveas) {
      toolbar->callbacks.on_file_saveas(userdata);
    }
    return true;

  case CMD_EDIT_UNDO:
    if (toolbar->callbacks.on_edit_undo) {
      toolbar->callbacks.on_edit_undo(userdata);
    }
    return true;

  case CMD_EDIT_REDO:
    if (toolbar->callbacks.on_edit_redo) {
      toolbar->callbacks.on_edit_redo(userdata);
    }
    return true;

  case CMD_EDIT_IMPORT_SCRIPTS:
    if (toolbar->callbacks.on_edit_import_scripts) {
      toolbar->callbacks.on_edit_import_scripts(userdata);
    }
    return true;

  case CMD_EDIT_CONVERT_ANM:
    if (toolbar->callbacks.on_edit_convert_anm) {
      toolbar->callbacks.on_edit_convert_anm(userdata);
    }
    return true;

  default:
    return false;
  }
}

bool anm2editor_toolbar_handle_notify(struct anm2editor_toolbar *const toolbar, void *const lparam) {
  if (!toolbar || !lparam) {
    return false;
  }

  NMHDR const *const nmhdr = (NMHDR const *)lparam;
  if (nmhdr->code != TTN_GETDISPINFOW) {
    return false;
  }

  NMTTDISPINFOW *const ttdi = (NMTTDISPINFOW *)lparam;
  char const *text = NULL;

  switch (ttdi->hdr.idFrom) {
  case CMD_FILE_NEW:
    text = pgettext("anm2editor", "New");
    break;
  case CMD_FILE_OPEN:
    text = pgettext("anm2editor", "Open");
    break;
  case CMD_FILE_SAVE:
    text = pgettext("anm2editor", "Save");
    break;
  case CMD_FILE_SAVEAS:
    text = pgettext("anm2editor", "Save As");
    break;
  case CMD_EDIT_UNDO:
    text = pgettext("anm2editor", "Undo");
    break;
  case CMD_EDIT_REDO:
    text = pgettext("anm2editor", "Redo");
    break;
  case CMD_EDIT_IMPORT_SCRIPTS:
    text = pgettext("anm2editor", "Import Scripts from Selected Object in AviUtl");
    break;
  case CMD_EDIT_CONVERT_ANM:
    text = pgettext("anm2editor", "Convert Old Animation Script(*.anm) to New(*.anm2)");
    break;
  default:
    return false;
  }

  if (text) {
    ov_utf8_to_wchar(text, strlen(text), ttdi->szText, sizeof(ttdi->szText) / sizeof(ttdi->szText[0]), NULL);
    ttdi->lpszText = ttdi->szText;
  }

  return true;
}
