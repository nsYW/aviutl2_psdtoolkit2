#include "dialog.h"

#include <ovl/os.h>

#include "win32.h"

int ptk_dialog_show(struct ptk_dialog_params const *const params) {
  enum {
    max_custom_buttons = 8,
  };
  if (!params || params->custom_button_count > max_custom_buttons) {
    return 0;
  }

  HANDLE hActCtx = INVALID_HANDLE_VALUE;
  ULONG_PTR cookie = 0;
  bool activated = false;
  HWND *disabled_windows = NULL;
  int button_id = 0;
  void *dll_hinst = NULL;
  ACTCTXW actctx = {0};
  TASKDIALOGCONFIG config = {0};
  TASKDIALOG_BUTTON btn_array[max_custom_buttons] = {0};

  // Get DLL instance handle for activation context
  if (!ovl_os_get_hinstance_from_fnptr((void *)ptk_dialog_show, &dll_hinst, NULL)) {
    goto cleanup;
  }

  // Create activation context for common controls
  actctx = (ACTCTXW){
      .cbSize = sizeof(ACTCTXW),
      .dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID,
      .lpResourceName = MAKEINTRESOURCEW(1),
      .hModule = (HMODULE)dll_hinst,
  };
  hActCtx = CreateActCtxW(&actctx);
  if (hActCtx == INVALID_HANDLE_VALUE) {
    goto cleanup;
  }
  if (!ActivateActCtx(hActCtx, &cookie)) {
    goto cleanup;
  }
  activated = true;

  // Build TASKDIALOGCONFIG
  config = (TASKDIALOGCONFIG){
      .cbSize = sizeof(TASKDIALOGCONFIG),
      .hwndParent = params->owner,
      .dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_EXPAND_FOOTER_AREA,
      .dwCommonButtons = params->buttons,
      .nDefaultButton = params->default_button,
      .pszWindowTitle = params->window_title,
      .pszMainIcon = params->icon,
      .pszMainInstruction = params->main_instruction,
      .pszContent = params->content,
      .pszExpandedInformation = params->expanded_info,
  };

  // Set up custom buttons if provided
  if (params->custom_buttons && params->custom_button_count > 0) {
    for (size_t i = 0; i < params->custom_button_count; i++) {
      btn_array[i].nButtonID = params->custom_buttons[i].id;
      btn_array[i].pszButtonText = params->custom_buttons[i].text;
    }
    config.dwCommonButtons = 0;
    config.pButtons = btn_array;
    config.cButtons = (UINT)params->custom_button_count;
  }

  // Disable other windows in the process
  disabled_windows = ptk_win32_disable_family_windows(params->owner);

  TaskDialogIndirect(&config, &button_id, NULL, NULL);

cleanup:
  ptk_win32_restore_disabled_family_windows(disabled_windows);
  if (activated) {
    DeactivateActCtx(0, cookie);
  }
  if (hActCtx != INVALID_HANDLE_VALUE) {
    ReleaseActCtx(hActCtx);
  }
  return button_id;
}
