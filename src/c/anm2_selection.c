#include "anm2_selection.h"

#include "anm2.h"

#include <ovarray.h>

struct anm2_selection {
  struct ptk_anm2 const *doc;
  enum anm2_selection_focus_type focus_type;
  uint32_t focus_id;
  uint32_t anchor_id;
  uint32_t *selected_item_ids; // ovarray
};

static bool
selection_resolve_item(struct anm2_selection const *sel, uint32_t item_id, size_t *sel_idx, size_t *item_idx) {
  if (!sel || !sel->doc) {
    return false;
  }
  return ptk_anm2_find_item(sel->doc, item_id, sel_idx, item_idx);
}

static bool selection_resolve_selector(struct anm2_selection const *sel, uint32_t selector_id, size_t *sel_idx) {
  if (!sel || !sel->doc) {
    return false;
  }
  return ptk_anm2_find_selector(sel->doc, selector_id, sel_idx);
}

static bool selection_contains(struct anm2_selection const *sel, uint32_t item_id) {
  if (!sel || item_id == 0 || !sel->selected_item_ids) {
    return false;
  }
  size_t const count = OV_ARRAY_LENGTH(sel->selected_item_ids);
  for (size_t i = 0; i < count; ++i) {
    if (sel->selected_item_ids[i] == item_id) {
      return true;
    }
  }
  return false;
}

static void selection_clear_ids(struct anm2_selection *sel) {
  if (sel->selected_item_ids) {
    OV_ARRAY_SET_LENGTH(sel->selected_item_ids, 0);
  }
}

static bool selection_push_id(struct anm2_selection *sel, uint32_t item_id, struct ov_error *err) {
  if (!OV_ARRAY_PUSH(&sel->selected_item_ids, item_id)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    return false;
  }
  return true;
}

static bool selection_add_unique(struct anm2_selection *sel, uint32_t item_id, struct ov_error *err) {
  if (item_id == 0 || selection_contains(sel, item_id)) {
    return true;
  }
  return selection_push_id(sel, item_id, err);
}

static void selection_set_focus(struct anm2_selection *sel, enum anm2_selection_focus_type type, uint32_t id) {
  sel->focus_type = type;
  sel->focus_id = id;
}

static void selection_refresh_anchor(struct anm2_selection *sel) {
  if (!sel || sel->anchor_id == 0) {
    return;
  }
  size_t sel_idx = 0;
  size_t item_idx = 0;
  if (!selection_resolve_item(sel, sel->anchor_id, &sel_idx, &item_idx)) {
    sel->anchor_id = 0;
  }
}

static void selection_refresh_focus(struct anm2_selection *sel) {
  if (!sel) {
    return;
  }
  if (sel->focus_type == anm2_selection_focus_selector) {
    size_t sel_idx = 0;
    if (!selection_resolve_selector(sel, sel->focus_id, &sel_idx)) {
      selection_set_focus(sel, anm2_selection_focus_none, 0);
    }
    return;
  }
  if (sel->focus_type == anm2_selection_focus_item) {
    size_t sel_idx = 0;
    size_t item_idx = 0;
    if (!selection_resolve_item(sel, sel->focus_id, &sel_idx, &item_idx)) {
      selection_set_focus(sel, anm2_selection_focus_none, 0);
    }
  }
}

static void selection_refresh_multisel(struct anm2_selection *sel) {
  if (!sel || !sel->selected_item_ids) {
    return;
  }
  size_t write_idx = 0;
  size_t const count = OV_ARRAY_LENGTH(sel->selected_item_ids);
  for (size_t i = 0; i < count; ++i) {
    uint32_t const item_id = sel->selected_item_ids[i];
    size_t item_sel = 0;
    size_t item_idx = 0;
    if (!selection_resolve_item(sel, item_id, &item_sel, &item_idx)) {
      continue;
    }
    sel->selected_item_ids[write_idx++] = item_id;
  }
  OV_ARRAY_SET_LENGTH(sel->selected_item_ids, write_idx);
  if (write_idx == 0 && sel->focus_type == anm2_selection_focus_item) {
    selection_set_focus(sel, anm2_selection_focus_none, 0);
    sel->anchor_id = 0;
  }
}

NODISCARD struct anm2_selection *anm2_selection_create(struct ptk_anm2 const *doc, struct ov_error *err) {
  struct anm2_selection *out = NULL;

  if (!doc) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return NULL;
  }

  if (!OV_REALLOC(&out, 1, sizeof(*out))) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    return NULL;
  }

  *out = (struct anm2_selection){
      .doc = doc,
  };
  return out;
}

void anm2_selection_destroy(struct anm2_selection **sel) {
  if (!sel || !*sel) {
    return;
  }
  struct anm2_selection *p = *sel;
  if (p->selected_item_ids) {
    OV_ARRAY_DESTROY(&p->selected_item_ids);
  }
  OV_FREE(sel);
}

void anm2_selection_clear(struct anm2_selection *sel) {
  if (!sel) {
    return;
  }
  selection_clear_ids(sel);
  sel->anchor_id = 0;
  selection_set_focus(sel, anm2_selection_focus_none, 0);
}

NODISCARD bool
anm2_selection_set_focus_selector(struct anm2_selection *sel, uint32_t selector_id, struct ov_error *err) {
  if (!sel || !sel->doc) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }
  if (selector_id == 0) {
    anm2_selection_clear(sel);
    return true;
  }
  size_t sel_idx = 0;
  if (!selection_resolve_selector(sel, selector_id, &sel_idx)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }
  selection_clear_ids(sel);
  sel->anchor_id = 0;
  selection_set_focus(sel, anm2_selection_focus_selector, selector_id);
  return true;
}

NODISCARD bool
anm2_selection_set_focus_item(struct anm2_selection *sel, uint32_t item_id, bool update_anchor, struct ov_error *err) {
  if (!sel || !sel->doc) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }
  if (item_id == 0) {
    anm2_selection_clear(sel);
    return true;
  }
  size_t sel_idx = 0;
  size_t item_idx = 0;
  if (!selection_resolve_item(sel, item_id, &sel_idx, &item_idx)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }
  selection_clear_ids(sel);
  if (!selection_push_id(sel, item_id, err)) {
    return false;
  }
  if (update_anchor) {
    sel->anchor_id = item_id;
  }
  selection_set_focus(sel, anm2_selection_focus_item, item_id);
  return true;
}

static bool selection_apply_range(struct anm2_selection *sel, uint32_t from_id, uint32_t to_id, struct ov_error *err) {
  if (from_id == 0 || to_id == 0) {
    return true;
  }
  size_t from_sel = 0;
  size_t from_item = 0;
  size_t to_sel = 0;
  size_t to_item = 0;
  if (!selection_resolve_item(sel, from_id, &from_sel, &from_item)) {
    return true;
  }
  if (!selection_resolve_item(sel, to_id, &to_sel, &to_item)) {
    return true;
  }
  if (from_sel > to_sel || (from_sel == to_sel && from_item > to_item)) {
    size_t tmp_sel = from_sel;
    size_t tmp_item = from_item;
    from_sel = to_sel;
    from_item = to_item;
    to_sel = tmp_sel;
    to_item = tmp_item;
  }

  size_t const sel_count = ptk_anm2_selector_count(sel->doc);
  for (size_t sel_idx = from_sel; sel_idx <= to_sel && sel_idx < sel_count; ++sel_idx) {
    uint32_t const selector_id = ptk_anm2_selector_get_id(sel->doc, sel_idx);
    size_t const item_count = ptk_anm2_item_count(sel->doc, selector_id);
    size_t start_item = (sel_idx == from_sel) ? from_item : 0;
    size_t end_item = (sel_idx == to_sel) ? to_item : (item_count > 0 ? item_count - 1 : 0);
    for (size_t item_idx = start_item; item_idx <= end_item && item_idx < item_count; ++item_idx) {
      uint32_t const item_id = ptk_anm2_item_get_id(sel->doc, sel_idx, item_idx);
      if (!selection_add_unique(sel, item_id, err)) {
        return false;
      }
    }
  }
  return true;
}

NODISCARD bool anm2_selection_apply_treeview_selection(struct anm2_selection *sel,
                                                       uint32_t item_id,
                                                       bool is_selector,
                                                       bool ctrl_pressed,
                                                       bool shift_pressed,
                                                       struct ov_error *err) {
  if (!sel || !sel->doc) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  if (item_id == 0) {
    anm2_selection_clear(sel);
    return true;
  }

  if (is_selector) {
    if (ctrl_pressed) {
      size_t sel_idx = 0;
      if (!selection_resolve_selector(sel, item_id, &sel_idx)) {
        OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
        return false;
      }
      selection_set_focus(sel, anm2_selection_focus_selector, item_id);
      return true;
    }
    return anm2_selection_set_focus_selector(sel, item_id, err);
  }

  if (shift_pressed && sel->anchor_id != 0) {
    if (!ctrl_pressed) {
      selection_clear_ids(sel);
    }
    if (!selection_apply_range(sel, sel->anchor_id, item_id, err)) {
      return false;
    }
    selection_set_focus(sel, anm2_selection_focus_item, item_id);
    return true;
  }

  if (ctrl_pressed) {
    if (selection_contains(sel, item_id)) {
      size_t count = OV_ARRAY_LENGTH(sel->selected_item_ids);
      for (size_t i = 0; i < count; ++i) {
        if (sel->selected_item_ids[i] == item_id) {
          sel->selected_item_ids[i] = sel->selected_item_ids[count - 1];
          OV_ARRAY_SET_LENGTH(sel->selected_item_ids, count - 1);
          break;
        }
      }
      selection_set_focus(sel, anm2_selection_focus_item, item_id);
      return true;
    }
    if (!selection_add_unique(sel, item_id, err)) {
      return false;
    }
    sel->anchor_id = item_id;
    selection_set_focus(sel, anm2_selection_focus_item, item_id);
    return true;
  }

  return anm2_selection_set_focus_item(sel, item_id, true, err);
}

void anm2_selection_get_state(struct anm2_selection const *sel, struct anm2_selection_state *out) {
  if (!sel || !out) {
    return;
  }
  out->focus_type = sel->focus_type;
  out->focus_id = sel->focus_id;
  out->anchor_id = sel->anchor_id;
}

uint32_t const *anm2_selection_get_selected_ids(struct anm2_selection const *sel, size_t *count) {
  if (!sel || !sel->selected_item_ids) {
    if (count) {
      *count = 0;
    }
    return NULL;
  }
  size_t const length = OV_ARRAY_LENGTH(sel->selected_item_ids);
  if (length == 0) {
    if (count) {
      *count = 0;
    }
    return NULL;
  }
  if (count) {
    *count = length;
  }
  return sel->selected_item_ids;
}

size_t anm2_selection_get_selected_count(struct anm2_selection const *sel) {
  if (!sel || !sel->selected_item_ids) {
    return 0;
  }
  return OV_ARRAY_LENGTH(sel->selected_item_ids);
}

bool anm2_selection_is_selected(struct anm2_selection const *sel, uint32_t item_id) {
  return selection_contains(sel, item_id);
}

NODISCARD bool anm2_selection_replace_selected_items(struct anm2_selection *sel,
                                                     uint32_t const *item_ids,
                                                     size_t item_count,
                                                     uint32_t focus_id,
                                                     uint32_t anchor_id,
                                                     struct ov_error *err) {
  if (!sel) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }
  selection_clear_ids(sel);
  for (size_t i = 0; i < item_count; ++i) {
    if (!selection_add_unique(sel, item_ids[i], err)) {
      return false;
    }
  }
  sel->anchor_id = anchor_id;
  selection_set_focus(sel, focus_id == 0 ? anm2_selection_focus_none : anm2_selection_focus_item, focus_id);
  return true;
}

void anm2_selection_refresh(struct anm2_selection *sel) {
  if (!sel) {
    return;
  }
  selection_refresh_focus(sel);
  selection_refresh_anchor(sel);
  selection_refresh_multisel(sel);
}
