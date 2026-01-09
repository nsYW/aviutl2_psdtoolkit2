#pragma once

#include <ovbase.h>

struct aviutl2_edit_handle;
struct aviutl2_host_app_table;
struct aviutl2_project_file;
struct aviutl2_script_module_param;
struct ptk_anm2editor;
struct ptk_cache;
struct ptk_script_module;

struct psdtoolkit;

/**
 * @brief Create and initialize psdtoolkit context
 *
 * @param cache Cache instance (not owned, must outlive psdtoolkit)
 * @param err [out] Error information on failure
 * @return Pointer to created context, or NULL on failure
 */
NODISCARD struct psdtoolkit *psdtoolkit_create(struct ptk_cache *cache, struct ov_error *err);

/**
 * @brief Destroy psdtoolkit context
 *
 * @param ptk [in,out] Pointer to context to destroy, will be set to NULL
 */
void psdtoolkit_destroy(struct psdtoolkit **ptk);

/**
 * @brief Set the edit handle for the plugin context
 *
 * @param ptk Plugin context
 * @param edit Edit handle from AviUtl2 host
 */
void psdtoolkit_set_edit_handle(struct psdtoolkit *ptk, struct aviutl2_edit_handle *edit);

/**
 * @brief Set the PSDToolKit anm2 Editor instance for the plugin context
 *
 * @param ptk Plugin context
 * @param anm2editor PSDToolKit anm2 Editor instance (not owned, must outlive psdtoolkit)
 */
void psdtoolkit_set_anm2editor(struct psdtoolkit *ptk, struct ptk_anm2editor *anm2editor);

/**
 * @brief Get the script module instance
 *
 * @param ptk Plugin context
 * @return Script module instance, or NULL if ptk is NULL
 */
struct ptk_script_module *psdtoolkit_get_script_module(struct psdtoolkit *ptk);

/**
 * @brief Create the plugin window
 *
 * @param ptk Plugin context
 * @param title Window title
 * @param err [out] Error information on failure
 * @return Window handle on success, NULL on failure
 */
NODISCARD void *psdtoolkit_create_plugin_window(struct psdtoolkit *ptk, wchar_t const *title, struct ov_error *err);

/**
 * @brief Handle project load event
 *
 * @param ptk Plugin context
 * @param project Project file interface
 */
void psdtoolkit_project_load_handler(struct psdtoolkit *ptk, struct aviutl2_project_file *project);

/**
 * @brief Handle project save event
 *
 * @param ptk Plugin context
 * @param project Project file interface
 */
void psdtoolkit_project_save_handler(struct psdtoolkit *ptk, struct aviutl2_project_file *project);

/**
 * @brief Show configuration dialog
 *
 * @param ptk Plugin context
 * @param hwnd Parent window handle
 */
void psdtoolkit_show_config_dialog(struct psdtoolkit *ptk, void *hwnd);
