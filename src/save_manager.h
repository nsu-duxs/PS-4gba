#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <orbis/UserService.h>
#include "emulator_manager.h"

#define PS4_PSGBA_ROOT_DIR        "/data/PSgba"
#define PS4_PSGBA_USERS_DIR       "/data/PSgba/users"
#define PS4_PSGBA_STATES_DIR      "/data/PSgba/states"
#define PS4_PSGBA_SAVES_DIR       "/data/PSgba/saves"
#define PS4_PSGBA_GUEST_SAVES_DIR "/data/PSgba/saves/guests"

#define PS4_DATA_ROOT_DIR         "/data/gba4p"
#define PS4_ROMS_DIR              "/data/gba4p/roms"
#define PS4_SAVES_DIR             "/data/gba4p/saves"
#define PS4_GUEST_SAVES_DIR       "/data/gba4p/saves/guests"
#define PS4_STATES_DIR            "/data/gba4p/states"

#define SAVE_STATE_MIN_SLOT       1
#define SAVE_STATE_MAX_SLOT       3

#define SAVE_THUMB_WIDTH          GBA_SCREEN_WIDTH
#define SAVE_THUMB_HEIGHT         GBA_SCREEN_HEIGHT
#define SAVE_THUMB_PIXELS         (SAVE_THUMB_WIDTH * SAVE_THUMB_HEIGHT)

typedef enum PS4SaveStateMode {
    SAVE_MODE_INDIVIDUAL = 0,
    SAVE_MODE_GROUP      = 1
} PS4SaveStateMode;

typedef struct PS4SaveStatePreview {
    bool occupied;
    bool has_thumbnail;
    bool is_group;
    int slot;
    int player_id;
    int active_count;
    uint32_t user_id;
    char date_str[16];     /* DD/MM/AAAA */
    char time_str[16];     /* HH:MM:SS   */
    char rom_title[128];
} PS4SaveStatePreview;

bool save_manager_init(void);
uint32_t save_manager_get_active_user_id(void);
OrbisUserServiceUserId save_manager_get_user_id_for_player(int active_player_index);
void save_manager_extract_rom_name(const char* rom_path, char* out_name, size_t max_len);
bool save_manager_attach_battery_save(struct mCore* core, const char* rom_name, int player_id,
                                      OrbisUserServiceUserId user_id);
void save_manager_flush_battery_save(struct mCore* core);

PS4SaveStateMode save_manager_get_mode(void);
void save_manager_set_mode(PS4SaveStateMode mode);
void save_manager_toggle_mode(void);

int save_manager_get_indiv_slot(void);
void save_manager_set_indiv_slot(int slot);
int save_manager_get_group_slot(void);
void save_manager_set_group_slot(int slot);

int save_manager_get_slot(void);
void save_manager_set_slot(int slot);
void save_manager_cycle_slot(int delta);

bool save_manager_quick_save_state_slot(struct PS4GBA_Context* ctx, int player_id, int slot);
bool save_manager_quick_load_state_slot(struct PS4GBA_Context* ctx, int player_id, int slot);

bool save_manager_group_save_state_slot(struct PS4GBA_Context* ctx, int slot);
bool save_manager_group_load_state_slot(struct PS4GBA_Context* ctx, int slot);

bool save_manager_quick_save_state(struct PS4GBA_Context* ctx, int player_id);
bool save_manager_quick_load_state(struct PS4GBA_Context* ctx, int player_id);

bool save_manager_group_save_state(struct PS4GBA_Context* ctx);
bool save_manager_group_load_state(struct PS4GBA_Context* ctx);

bool save_manager_load_state_preview(struct PS4GBA_Context* ctx, int player_id, int slot,
                                     bool is_group, PS4SaveStatePreview* out_info,
                                     uint32_t* out_argb_pixels);

#endif /* SAVE_MANAGER_H */
