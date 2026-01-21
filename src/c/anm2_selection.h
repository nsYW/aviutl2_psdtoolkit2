#pragma once

#include <ovbase.h>

struct ptk_anm2;
struct anm2_selection;

enum anm2_selection_focus_type {
  anm2_selection_focus_none,
  anm2_selection_focus_selector,
  anm2_selection_focus_item,
};

struct anm2_selection_state {
  uint32_t focus_id;
  uint32_t anchor_id;
  enum anm2_selection_focus_type focus_type;
};

NODISCARD struct anm2_selection *anm2_selection_create(struct ptk_anm2 const *doc, struct ov_error *err);
void anm2_selection_destroy(struct anm2_selection **sel);

void anm2_selection_clear(struct anm2_selection *sel);

NODISCARD bool
anm2_selection_set_focus_selector(struct anm2_selection *sel, uint32_t selector_id, struct ov_error *err);
NODISCARD bool
anm2_selection_set_focus_item(struct anm2_selection *sel, uint32_t item_id, bool update_anchor, struct ov_error *err);

NODISCARD bool anm2_selection_apply_treeview_selection(struct anm2_selection *sel,
                                                       uint32_t item_id,
                                                       bool is_selector,
                                                       bool ctrl_pressed,
                                                       bool shift_pressed,
                                                       struct ov_error *err);

void anm2_selection_get_state(struct anm2_selection const *sel, struct anm2_selection_state *out);
uint32_t const *anm2_selection_get_selected_ids(struct anm2_selection const *sel, size_t *count);
size_t anm2_selection_get_selected_count(struct anm2_selection const *sel);
bool anm2_selection_is_selected(struct anm2_selection const *sel, uint32_t item_id);

NODISCARD bool anm2_selection_replace_selected_items(struct anm2_selection *sel,
                                                     uint32_t const *item_ids,
                                                     size_t item_count,
                                                     uint32_t focus_id,
                                                     uint32_t anchor_id,
                                                     struct ov_error *err);

void anm2_selection_refresh(struct anm2_selection *sel);
