#include "save_manager.h"
#include "input_ds4.h"
#include "link_cable_hub.h"
#include "ps4_jailbreak.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/savedata.h>

#define GROUP_STATE_MAGIC   0x34534750U /* 'PGS4' */
#define SAVEDATA_SNAP_MAGIC 0x56415350U /* 'PSAV' */
#define THUMB_FILE_MAGIC    0x31424D54U /* 'TMB1' */

typedef struct PS4SavedataSnapshotHeader {
    uint32_t magic;
    int32_t savedata_type;
    uint32_t savedata_size;
} PS4SavedataSnapshotHeader;

typedef struct PS4ThumbnailHeader {
    uint32_t magic;
    uint16_t width;
    uint16_t height;
    int64_t timestamp_unix;
    int32_t slot;
    int32_t player_id;
    int32_t is_group;
    int32_t active_count;
    char date_str[16];     /* DD/MM/AAAA */
    char time_str[16];     /* HH:MM:SS   */
    char rom_title[128];
} PS4ThumbnailHeader;

typedef struct PS4GroupStateMeta {
    uint32_t magic;
    int32_t active_count;
    uint32_t active_mask;
    uint32_t single_state_size;
    uint16_t multi_data[LINK_MAX_PLAYERS];
    uint16_t pending_multi_send[LINK_MAX_PLAYERS];
    uint8_t normal8_data[LINK_MAX_PLAYERS];
    uint32_t normal32_data[LINK_MAX_PLAYERS];
    int32_t transfer_mode;
    uint8_t cable_enabled;
    uint8_t reserved[3];
    int32_t sio_mode[LINK_MAX_PLAYERS];
    uint16_t sio_rcnt[LINK_MAX_PLAYERS];
    uint16_t sio_siocnt[LINK_MAX_PLAYERS];
    uint8_t has_entered_multi[LINK_MAX_PLAYERS];
    uint32_t gpio_grace_frames[LINK_MAX_PLAYERS];
} PS4GroupStateMeta;

static PS4SaveStateMode s_save_mode = SAVE_MODE_INDIVIDUAL;
static int s_indiv_slot = 1;
static int s_group_slot = 1;

OrbisUserServiceUserId save_manager_get_user_id_for_player(int active_player_index) {
    int idx = (active_player_index >= 0 && active_player_index < GBA_MAX_INSTANCES)
                  ? active_player_index
                  : 0;

    OrbisUserServiceLoginUserIdList user_list;
    memset(&user_list, 0, sizeof(user_list));
    if (sceUserServiceGetLoginUserIdList(&user_list) >= 0) {
        if (user_list.userId[idx] > 0 &&
            user_list.userId[idx] != ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
            return user_list.userId[idx];
        }
    }

    OrbisUserServiceUserId pad_uid = input_get_user_id(idx);
    if (pad_uid > 0 && pad_uid != ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        return pad_uid;
    }

    if (g_gba_ctx.instances[idx].user_id > 0 &&
        g_gba_ctx.instances[idx].user_id != ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        return g_gba_ctx.instances[idx].user_id;
    }

    if (idx == 0) {
        OrbisUserServiceUserId initial_uid = -1;
        if (sceUserServiceGetInitialUser(&initial_uid) >= 0 &&
            initial_uid > 0 &&
            initial_uid != ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
            return initial_uid;
        }
        return (OrbisUserServiceUserId)0x10000001;
    }

    return (OrbisUserServiceUserId)(0x20000000 + idx);
}

uint32_t save_manager_get_active_user_id(void) {
    OrbisUserServiceUserId user_id = -1;
    if (sceUserServiceGetInitialUser(&user_id) >= 0 &&
        user_id > 0 &&
        user_id != ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        return (uint32_t)user_id;
    }

    return (uint32_t)save_manager_get_user_id_for_player(0);
}

static inline uint32_t xbgr_to_argb_pixel(uint32_t px) {
    uint32_t r = (px & 0x000000FFU) << 16;
    uint32_t g = (px & 0x0000FF00U);
    uint32_t b = (px & 0x00FF0000U) >> 16;
    return 0xFF000000U | r | g | b;
}

static void format_unix_timestamp(int64_t ts, char* out_date, size_t date_max,
                                  char* out_time, size_t time_max) {
    if (out_date && date_max > 0) {
        snprintf(out_date, date_max, "--/--/----");
    }
    if (out_time && time_max > 0) {
        snprintf(out_time, time_max, "--:--:--");
    }
    if (ts <= 0) {
        return;
    }

    /*
     * Conversao calendario civil direta (algoritmo de Howard Hinnant / rata die)
     * para evitar dependencia de tzdata ou .tbss em localtime() no Orbis OS.
     */
    int64_t secs = ts;
    int64_t days = secs / 86400;
    int64_t rem = secs % 86400;
    if (rem < 0) {
        rem += 86400;
        days -= 1;
    }

    int hour = (int)(rem / 3600);
    rem %= 3600;
    int min = (int)(rem / 60);
    int sec = (int)(rem % 60);

    int64_t z = days + 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t y = (int64_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t d = doy - (153 * mp + 2) / 5 + 1;
    uint32_t m = mp < 10 ? mp + 3 : mp - 9;
    y += (m <= 2);

    if (out_date && date_max > 0) {
        snprintf(out_date, date_max, "%02u/%02u/%04d", d, m, (int)y);
    }
    if (out_time && time_max > 0) {
        snprintf(out_time, time_max, "%02d:%02d:%02d", hour, min, sec);
    }
}

static void ensure_dir_recursive(const char* dir_path) {
    if (!dir_path || !*dir_path) {
        return;
    }

    char tmp[256];
    strncpy(tmp, dir_path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    size_t len = strlen(tmp);
    if (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (char* p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0777);
            *p = '/';
        }
    }
    mkdir(tmp, 0777);
}

static void ensure_user_states_dir(uint32_t uid, char* out_dir, size_t max_len) {
    ps4_jailbreak_system();
    ensure_dir_recursive("/data");
    ensure_dir_recursive(PS4_PSGBA_ROOT_DIR);
    ensure_dir_recursive(PS4_PSGBA_USERS_DIR);

    char user_root[256];
    snprintf(user_root, sizeof(user_root), "%s/0x%08X", PS4_PSGBA_USERS_DIR, uid);
    ensure_dir_recursive(user_root);

    char user_states[256];
    snprintf(user_states, sizeof(user_states), "%s/states", user_root);
    ensure_dir_recursive(user_states);

    char user_saves[256];
    snprintf(user_saves, sizeof(user_saves), "%s/saves", user_root);
    ensure_dir_recursive(user_saves);

    if (out_dir && max_len > 0) {
        strncpy(out_dir, user_states, max_len - 1);
        out_dir[max_len - 1] = '\0';
    }
}

static void ensure_all_save_directories(void) {
    ps4_jailbreak_system();
    ensure_dir_recursive("/data");
    ensure_dir_recursive(PS4_PSGBA_ROOT_DIR);
    ensure_dir_recursive(PS4_PSGBA_USERS_DIR);
    ensure_dir_recursive(PS4_PSGBA_STATES_DIR);
    ensure_dir_recursive(PS4_PSGBA_SAVES_DIR);
    ensure_dir_recursive(PS4_PSGBA_GUEST_SAVES_DIR);

    ensure_dir_recursive(PS4_DATA_ROOT_DIR);
    ensure_dir_recursive(PS4_ROMS_DIR);
    ensure_dir_recursive(PS4_SAVES_DIR);
    ensure_dir_recursive(PS4_GUEST_SAVES_DIR);
    ensure_dir_recursive(PS4_STATES_DIR);

    ensure_user_states_dir(save_manager_get_active_user_id(), NULL, 0);
}

static void build_user_save_path(const char* rom_name, int player_id,
                                 OrbisUserServiceUserId user_id,
                                 char* out_path, size_t max_len) {
    ensure_all_save_directories();

    uint32_t active_uid = (user_id > 0 && user_id != ORBIS_USER_SERVICE_USER_ID_SYSTEM)
                              ? (uint32_t)user_id
                              : (uint32_t)save_manager_get_user_id_for_player(player_id);

    char user_saves_dir[256];
    snprintf(user_saves_dir, sizeof(user_saves_dir),
             "%s/0x%08X/saves", PS4_PSGBA_USERS_DIR, active_uid);
    ensure_dir_recursive(user_saves_dir);

    snprintf(out_path, max_len, "%s/%s.sav", user_saves_dir, rom_name);
}

static void build_individual_state_path(const char* rom_name, int slot, int player_id,
                                        char* out_path, size_t max_len) {
    uint32_t uid = (uint32_t)save_manager_get_user_id_for_player(player_id);
    char states_dir[256];
    ensure_user_states_dir(uid, states_dir, sizeof(states_dir));
    int clamped_slot = (slot >= SAVE_STATE_MIN_SLOT && slot <= SAVE_STATE_MAX_SLOT) ? slot : 1;
    const char* safe_name = (rom_name && *rom_name) ? rom_name : "rom";
    snprintf(out_path, max_len, "%s/%s_indiv_slot%d.ss0",
             states_dir, safe_name, clamped_slot);
}

static void build_individual_thumb_path(const char* rom_name, int slot, int player_id,
                                        char* out_path, size_t max_len) {
    uint32_t uid = (uint32_t)save_manager_get_user_id_for_player(player_id);
    char states_dir[256];
    ensure_user_states_dir(uid, states_dir, sizeof(states_dir));
    int clamped_slot = (slot >= SAVE_STATE_MIN_SLOT && slot <= SAVE_STATE_MAX_SLOT) ? slot : 1;
    const char* safe_name = (rom_name && *rom_name) ? rom_name : "rom";
    snprintf(out_path, max_len, "%s/%s_indiv_slot%d.thumb",
             states_dir, safe_name, clamped_slot);
}

static void build_individual_meta_path(const char* rom_name, int slot, int player_id,
                                       char* out_path, size_t max_len) {
    uint32_t uid = (uint32_t)save_manager_get_user_id_for_player(player_id);
    char states_dir[256];
    ensure_user_states_dir(uid, states_dir, sizeof(states_dir));
    int clamped_slot = (slot >= SAVE_STATE_MIN_SLOT && slot <= SAVE_STATE_MAX_SLOT) ? slot : 1;
    const char* safe_name = (rom_name && *rom_name) ? rom_name : "rom";
    snprintf(out_path, max_len, "%s/%s_indiv_slot%d.meta",
             states_dir, safe_name, clamped_slot);
}

static void resolve_existing_individual_paths(const char* rom_name, int slot, int player_id,
                                              char* out_state_path, size_t state_max,
                                              char* out_thumb_path, size_t thumb_max) {
    build_individual_state_path(rom_name, slot, player_id, out_state_path, state_max);
    if (out_thumb_path && thumb_max > 0) {
        build_individual_thumb_path(rom_name, slot, player_id, out_thumb_path, thumb_max);
    }

    struct stat st;
    if (stat(out_state_path, &st) == 0) {
        return;
    }

    /* Compatibilidade com saves da mesma conta gravados anteriormente com sufixo _p0.._p3 */
    uint32_t uid = (uint32_t)save_manager_get_user_id_for_player(player_id);
    char states_dir[256];
    ensure_user_states_dir(uid, states_dir, sizeof(states_dir));
    int clamped_slot = (slot >= SAVE_STATE_MIN_SLOT && slot <= SAVE_STATE_MAX_SLOT) ? slot : 1;
    const char* safe_name = (rom_name && *rom_name) ? rom_name : "rom";

    for (int p = 0; p < GBA_MAX_INSTANCES; ++p) {
        char cand_state[256];
        snprintf(cand_state, sizeof(cand_state), "%s/%s_indiv_slot%d_p%d.ss0",
                 states_dir, safe_name, clamped_slot, p);
        if (stat(cand_state, &st) == 0) {
            strncpy(out_state_path, cand_state, state_max - 1);
            out_state_path[state_max - 1] = '\0';
            if (out_thumb_path && thumb_max > 0) {
                snprintf(out_thumb_path, thumb_max, "%s/%s_indiv_slot%d_p%d.thumb",
                         states_dir, safe_name, clamped_slot, p);
            }
            return;
        }
    }
}

static void build_group_thumb_path(const char* rom_name, int slot,
                                   char* out_path, size_t max_len) {
    char states_dir[256];
    ensure_user_states_dir(save_manager_get_active_user_id(), states_dir, sizeof(states_dir));
    int clamped_slot = (slot >= SAVE_STATE_MIN_SLOT && slot <= SAVE_STATE_MAX_SLOT) ? slot : 1;
    const char* safe_name = (rom_name && *rom_name) ? rom_name : "rom";
    snprintf(out_path, max_len, "%s/%s_group_slot%d.thumb",
             states_dir, safe_name, clamped_slot);
}

static void build_group_state_paths(const char* rom_name, int slot,
                                    char* out_meta_path, size_t meta_max,
                                    char out_player_paths[GBA_MAX_INSTANCES][256]) {
    char states_dir[256];
    ensure_user_states_dir(save_manager_get_active_user_id(), states_dir, sizeof(states_dir));
    int clamped_slot = (slot >= SAVE_STATE_MIN_SLOT && slot <= SAVE_STATE_MAX_SLOT) ? slot : 1;
    const char* safe_name = (rom_name && *rom_name) ? rom_name : "rom";

    if (out_meta_path && meta_max > 0) {
        snprintf(out_meta_path, meta_max, "%s/%s_group_slot%d.meta",
                 states_dir, safe_name, clamped_slot);
    }
    if (out_player_paths) {
        for (int i = 0; i < GBA_MAX_INSTANCES; ++i) {
            snprintf(out_player_paths[i], 256, "%s/%s_group_slot%d_p%d.gss",
                     states_dir, safe_name, clamped_slot, i);
        }
    }
}

static void capture_and_write_thumbnail(struct PS4GBA_Context* ctx, int player_id, int slot,
                                        bool is_group, const char* thumb_path,
                                        const char* meta_sidecar_path) {
    if (!ctx || !thumb_path || !*thumb_path) {
        return;
    }

    uint32_t pixels[SAVE_THUMB_PIXELS];
    for (int i = 0; i < SAVE_THUMB_PIXELS; ++i) {
        pixels[i] = 0xFF080C14U;
    }

    if (is_group && ctx->active_count > 1) {
        /*
         * Miniatura 4-Way Split-Screen (240x160 composta pelos 4 quadrantes 120x80)
         * quando ha multiplos jogadores ativos na sessao.
         */
        int half_w = SAVE_THUMB_WIDTH / 2;   /* 120 */
        int half_h = SAVE_THUMB_HEIGHT / 2;  /* 80  */
        for (int p = 0; p < GBA_MAX_INSTANCES; ++p) {
            int qx = (p & 1) ? half_w : 0;
            int qy = (p >= 2) ? half_h : 0;
            const uint32_t* vbuf = (ctx->instances[p].active && ctx->instances[p].video_buffer)
                                       ? ctx->instances[p].video_buffer
                                       : NULL;
            for (int y = 0; y < half_h; ++y) {
                int sy = y * 2;
                uint32_t* dst_row = &pixels[(qy + y) * SAVE_THUMB_WIDTH + qx];
                if (vbuf) {
                    const uint32_t* src_row = &vbuf[sy * GBA_SCREEN_WIDTH];
                    for (int x = 0; x < half_w; ++x) {
                        dst_row[x] = xbgr_to_argb_pixel(src_row[x * 2]);
                    }
                } else {
                    for (int x = 0; x < half_w; ++x) {
                        dst_row[x] = 0xFF0A101CU;
                    }
                }
            }
        }
        /* Cruz divisoria ciano entre as 4 mini-telas */
        for (int x = 0; x < SAVE_THUMB_WIDTH; ++x) {
            pixels[half_h * SAVE_THUMB_WIDTH + x] = 0xFF00F5D4U;
        }
        for (int y = 0; y < SAVE_THUMB_HEIGHT; ++y) {
            pixels[y * SAVE_THUMB_WIDTH + half_w] = 0xFF00F5D4U;
        }
    } else {
        int src_p = (player_id >= 0 && player_id < GBA_MAX_INSTANCES) ? player_id : 0;
        if (!ctx->instances[src_p].active || !ctx->instances[src_p].video_buffer) {
            src_p = 0;
        }
        const uint32_t* vbuf = ctx->instances[src_p].video_buffer;
        if (vbuf) {
            for (int i = 0; i < SAVE_THUMB_PIXELS; ++i) {
                pixels[i] = xbgr_to_argb_pixel(vbuf[i]);
            }
        }
    }

    PS4ThumbnailHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = THUMB_FILE_MAGIC;
    hdr.width = (uint16_t)SAVE_THUMB_WIDTH;
    hdr.height = (uint16_t)SAVE_THUMB_HEIGHT;
    hdr.timestamp_unix = (int64_t)time(NULL);
    hdr.slot = slot;
    hdr.player_id = player_id;
    hdr.is_group = is_group ? 1 : 0;
    hdr.active_count = (ctx->active_count > 0) ? ctx->active_count : 1;

    format_unix_timestamp(hdr.timestamp_unix,
                          hdr.date_str, sizeof(hdr.date_str),
                          hdr.time_str, sizeof(hdr.time_str));

    const char* rname = (!is_group && player_id >= 0 && player_id < GBA_MAX_INSTANCES &&
                         ctx->instances[player_id].rom_name[0])
                            ? ctx->instances[player_id].rom_name
                            : ctx->rom_name;
    strncpy(hdr.rom_title, rname ? rname : "ROM", sizeof(hdr.rom_title) - 1);
    hdr.rom_title[sizeof(hdr.rom_title) - 1] = '\0';

    FILE* tf = fopen(thumb_path, "wb");
    if (tf) {
        fwrite(&hdr, 1, sizeof(hdr), tf);
        fwrite(pixels, sizeof(uint32_t), SAVE_THUMB_PIXELS, tf);
        fflush(tf);
        fclose(tf);
    }

    if (meta_sidecar_path && *meta_sidecar_path) {
        FILE* mf = fopen(meta_sidecar_path, "wb");
        if (mf) {
            fwrite(&hdr, 1, sizeof(hdr), mf);
            fflush(mf);
            fclose(mf);
        }
    }
}

bool save_manager_init(void) {
    ensure_all_save_directories();
    return true;
}

static int clamp_slot_value(int slot) {
    if (slot < SAVE_STATE_MIN_SLOT) {
        return SAVE_STATE_MIN_SLOT;
    }
    if (slot > SAVE_STATE_MAX_SLOT) {
        return SAVE_STATE_MAX_SLOT;
    }
    return slot;
}

PS4SaveStateMode save_manager_get_mode(void) {
    return s_save_mode;
}

void save_manager_set_mode(PS4SaveStateMode mode) {
    s_save_mode = (mode == SAVE_MODE_GROUP) ? SAVE_MODE_GROUP : SAVE_MODE_INDIVIDUAL;
}

void save_manager_toggle_mode(void) {
    s_save_mode = (s_save_mode == SAVE_MODE_INDIVIDUAL) ? SAVE_MODE_GROUP : SAVE_MODE_INDIVIDUAL;
}

int save_manager_get_indiv_slot(void) {
    return s_indiv_slot;
}

void save_manager_set_indiv_slot(int slot) {
    s_indiv_slot = clamp_slot_value(slot);
}

int save_manager_get_group_slot(void) {
    return s_group_slot;
}

void save_manager_set_group_slot(int slot) {
    s_group_slot = clamp_slot_value(slot);
}

int save_manager_get_slot(void) {
    return (s_save_mode == SAVE_MODE_GROUP) ? s_group_slot : s_indiv_slot;
}

void save_manager_set_slot(int slot) {
    if (s_save_mode == SAVE_MODE_GROUP) {
        s_group_slot = clamp_slot_value(slot);
    } else {
        s_indiv_slot = clamp_slot_value(slot);
    }
}

void save_manager_cycle_slot(int delta) {
    int count = (SAVE_STATE_MAX_SLOT - SAVE_STATE_MIN_SLOT) + 1;
    int cur = save_manager_get_slot();
    int idx = (cur - SAVE_STATE_MIN_SLOT + delta) % count;
    if (idx < 0) {
        idx += count;
    }
    save_manager_set_slot(SAVE_STATE_MIN_SLOT + idx);
}

void save_manager_extract_rom_name(const char* rom_path, char* out_name, size_t max_len) {
    if (!out_name || max_len == 0) {
        return;
    }
    if (!rom_path || !*rom_path) {
        strncpy(out_name, "rom", max_len - 1);
        out_name[max_len - 1] = '\0';
        return;
    }

    const char* slash = strrchr(rom_path, '/');
    const char* bslash = strrchr(rom_path, '\\');
    const char* base = rom_path;

    if (slash && bslash) {
        base = (slash > bslash) ? (slash + 1) : (bslash + 1);
    } else if (slash) {
        base = slash + 1;
    } else if (bslash) {
        base = bslash + 1;
    }

    const char* dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    if (len >= max_len) {
        len = max_len - 1;
    }

    memcpy(out_name, base, len);
    out_name[len] = '\0';
}

bool save_manager_attach_battery_save(struct mCore* core, const char* rom_name, int player_id,
                                      OrbisUserServiceUserId user_id) {
    if (!core || !rom_name || player_id < 0 || player_id >= GBA_MAX_INSTANCES) {
        return false;
    }

    char save_path[256];
    build_user_save_path(rom_name, player_id, user_id, save_path, sizeof(save_path));

    /*
     * Se o save ainda nao existe no PS4, verifica se ha um .sav pre-existente no Pendrive
     * ao lado da ROM para importar (ex: save trazido do PC). Apos importado, todas as
     * leituras e gravacoes permanecem estritamente no armazenamento interno do PS4.
     */
    struct stat st;
    if (stat(save_path, &st) != 0 || st.st_size == 0) {
        if (g_gba_ctx.rom_path[0] != '\0') {
            char usb_sav_path[256];
            strncpy(usb_sav_path, g_gba_ctx.rom_path, sizeof(usb_sav_path) - 1);
            usb_sav_path[sizeof(usb_sav_path) - 1] = '\0';
            char* dot = strrchr(usb_sav_path, '.');
            if (dot) {
                *dot = '\0';
                strncat(usb_sav_path, ".sav", sizeof(usb_sav_path) - strlen(usb_sav_path) - 1);
                FILE* src_f = fopen(usb_sav_path, "rb");
                if (src_f) {
                    FILE* dst_f = fopen(save_path, "wb");
                    if (dst_f) {
                        char buf[4096];
                        size_t n;
                        while ((n = fread(buf, 1, sizeof(buf), src_f)) > 0) {
                            fwrite(buf, 1, n, dst_f);
                        }
                        fflush(dst_f);
                        fclose(dst_f);
                    }
                    fclose(src_f);
                }
            }
        }
    }

    /*
     * Garante que o arquivo exista fisicamente antes de VFileOpen(..., O_CREAT | O_RDWR),
     * pois VFileFOpen abre em modo "r+b" e no OpenOrbis errno pode nao ser ENOENT.
     */
    if (stat(save_path, &st) != 0) {
        FILE* touch_f = fopen(save_path, "wb");
        if (touch_f) {
            fclose(touch_f);
        }
    }

    struct VFile* vf = VFileOpen(save_path, O_CREAT | O_RDWR);
    if (!vf) {
        return false;
    }

    return core->loadSave(core, vf);
}

void save_manager_flush_battery_save(struct mCore* core) {
    if (!core || !core->board) {
        return;
    }

    struct GBA* gba = (struct GBA*)core->board;
    struct GBASavedata* savedata = &gba->memory.savedata;
    if (!savedata->vf || !savedata->data) {
        return;
    }

    GBASavedataClean(savedata, 0);
    size_t size = GBASavedataSize(savedata);
    if (size > 0) {
        savedata->vf->sync(savedata->vf, savedata->data, size);
    }
}

static bool serialize_core_to_file(struct mCore* core, int slot_index, const char* file_path) {
    if (!core || !core->board || !file_path || !*file_path) {
        return false;
    }

    emulator_prepare_instance_for_save(slot_index);
    save_manager_flush_battery_save(core);

    size_t state_size = core->stateSize(core);
    if (state_size == 0) {
        return false;
    }

    void* state_buf = malloc(state_size);
    if (!state_buf) {
        return false;
    }
    memset(state_buf, 0, state_size);

    if (!core->saveState(core, state_buf)) {
        free(state_buf);
        return false;
    }

    FILE* fp = fopen(file_path, "wb");
    if (!fp) {
        free(state_buf);
        return false;
    }

    size_t written = fwrite(state_buf, 1, state_size, fp);
    free(state_buf);
    if (written != state_size) {
        fclose(fp);
        return false;
    }

    /*
     * Salva tambem o snapshot da memoria de cartucho (SRAM / Flash / EEPROM)
     * para garantir consistencia total entre a RAM do jogo e o Save State.
     */
    struct GBA* gba = (struct GBA*)core->board;
    struct GBASavedata* savedata = &gba->memory.savedata;
    PS4SavedataSnapshotHeader snap_hdr;
    memset(&snap_hdr, 0, sizeof(snap_hdr));
    snap_hdr.magic = SAVEDATA_SNAP_MAGIC;
    snap_hdr.savedata_type = (int32_t)savedata->type;

    size_t sram_size = (savedata->data && savedata->type != GBA_SAVEDATA_AUTODETECT)
                           ? GBASavedataSize(savedata)
                           : 0;
    if (sram_size > 0x20000U) {
        sram_size = 0;
    }
    snap_hdr.savedata_size = (uint32_t)sram_size;

    if (fwrite(&snap_hdr, 1, sizeof(snap_hdr), fp) == sizeof(snap_hdr) && sram_size > 0) {
        fwrite(savedata->data, 1, sram_size, fp);
    }

    fflush(fp);
    fclose(fp);
    return true;
}

static bool deserialize_core_from_file(struct mCore* core, int slot_index, const char* file_path) {
    if (!core || !core->board || !file_path || !*file_path) {
        return false;
    }

    size_t state_size = core->stateSize(core);
    if (state_size == 0) {
        return false;
    }

    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        return false;
    }

    void* state_buf = malloc(state_size);
    if (!state_buf) {
        fclose(fp);
        return false;
    }

    size_t bytes_read = fread(state_buf, 1, state_size, fp);
    if (bytes_read != state_size) {
        free(state_buf);
        fclose(fp);
        return false;
    }

    PS4SavedataSnapshotHeader snap_hdr;
    memset(&snap_hdr, 0, sizeof(snap_hdr));
    uint8_t* sram_buf = NULL;
    if (fread(&snap_hdr, 1, sizeof(snap_hdr), fp) == sizeof(snap_hdr) &&
        snap_hdr.magic == SAVEDATA_SNAP_MAGIC &&
        snap_hdr.savedata_size > 0 &&
        snap_hdr.savedata_size <= 0x20000U) {
        sram_buf = (uint8_t*)malloc(snap_hdr.savedata_size);
        if (sram_buf) {
            if (fread(sram_buf, 1, snap_hdr.savedata_size, fp) != snap_hdr.savedata_size) {
                free(sram_buf);
                sram_buf = NULL;
            }
        }
    }
    fclose(fp);

    emulator_prepare_instance_for_save(slot_index);
    bool ok = core->loadState(core, state_buf);
    free(state_buf);

    if (!ok) {
        if (sram_buf) {
            free(sram_buf);
        }
        emulator_restore_instance_after_state(slot_index);
        return false;
    }

    /* Restaura o snapshot de SRAM/Flash/EEPROM se presente */
    if (sram_buf) {
        struct GBA* gba = (struct GBA*)core->board;
        struct GBASavedata* savedata = &gba->memory.savedata;
        enum GBASavedataType target_type = (enum GBASavedataType)snap_hdr.savedata_type;
        if (target_type != GBA_SAVEDATA_AUTODETECT && savedata->type != target_type) {
            GBASavedataForceType(savedata, target_type);
        }
        size_t cur_size = GBASavedataSize(savedata);
        if (savedata->data && cur_size >= snap_hdr.savedata_size) {
            memcpy(savedata->data, sram_buf, snap_hdr.savedata_size);
            if (savedata->vf) {
                savedata->vf->sync(savedata->vf, savedata->data, snap_hdr.savedata_size);
            }
        }
        free(sram_buf);
    }

    emulator_restore_instance_after_state(slot_index);
    return true;
}

bool save_manager_quick_save_state_slot(struct PS4GBA_Context* ctx, int player_id, int slot) {
    if (!ctx || player_id < 0 || player_id >= GBA_MAX_INSTANCES) {
        return false;
    }

    struct mCore* core = ctx->instances[player_id].core;
    if (!ctx->instances[player_id].active || !core) {
        return false;
    }

    ps4_jailbreak_system();
    emulator_pause_workers();

    const char* rname = ctx->instances[player_id].rom_name[0]
                            ? ctx->instances[player_id].rom_name
                            : ctx->rom_name;
    char state_path[256];
    char thumb_path[256];
    char meta_path[256];
    build_individual_state_path(rname, slot, player_id, state_path, sizeof(state_path));
    build_individual_thumb_path(rname, slot, player_id, thumb_path, sizeof(thumb_path));
    build_individual_meta_path(rname, slot, player_id, meta_path, sizeof(meta_path));

    bool ok = serialize_core_to_file(core, player_id, state_path);
    if (ok) {
        capture_and_write_thumbnail(ctx, player_id, slot, false, thumb_path, meta_path);
    }
    link_hub_sync_pins();
    return ok;
}

bool save_manager_quick_load_state_slot(struct PS4GBA_Context* ctx, int player_id, int slot) {
    if (!ctx || player_id < 0 || player_id >= GBA_MAX_INSTANCES) {
        return false;
    }

    struct mCore* core = ctx->instances[player_id].core;
    if (!ctx->instances[player_id].active || !core) {
        return false;
    }

    ps4_jailbreak_system();
    emulator_pause_workers();

    const char* rname = ctx->instances[player_id].rom_name[0]
                            ? ctx->instances[player_id].rom_name
                            : ctx->rom_name;
    char state_path[256];
    resolve_existing_individual_paths(rname, slot, player_id, state_path, sizeof(state_path), NULL, 0);

    struct stat st;
    if (stat(state_path, &st) != 0) {
        return false;
    }

    bool ok = deserialize_core_from_file(core, player_id, state_path);
    if (ok) {
        struct GBA* gba = (struct GBA*)core->board;
        if (gba) {
            /* Limpa eventual bit Busy/Start preso de transferencia anterior ao carregar individualmente */
            if (gba->sio.mode == GBA_SIO_MULTI) {
                gba->sio.siocnt = GBASIOMultiplayerClearBusy(gba->sio.siocnt);
                gba->memory.io[GBA_REG(SIOCNT)] = gba->sio.siocnt;
            } else if (gba->sio.mode == GBA_SIO_NORMAL_8 || gba->sio.mode == GBA_SIO_NORMAL_32) {
                gba->sio.siocnt = GBASIONormalClearStart(gba->sio.siocnt);
                gba->memory.io[GBA_REG(SIOCNT)] = gba->sio.siocnt;
            }
        }
        link_hub_sync_pins();
    }
    return ok;
}

bool save_manager_group_save_state_slot(struct PS4GBA_Context* ctx, int slot) {
    if (!ctx || ctx->active_count <= 0 || !ctx->instances[0].core) {
        return false;
    }

    ps4_jailbreak_system();
    emulator_pause_workers();

    size_t state_size = ctx->instances[0].core->stateSize(ctx->instances[0].core);
    if (state_size == 0) {
        return false;
    }

    char meta_path[256];
    char thumb_path[256];
    char player_paths[GBA_MAX_INSTANCES][256];
    build_group_state_paths(ctx->rom_name, slot, meta_path, sizeof(meta_path), player_paths);
    build_group_thumb_path(ctx->rom_name, slot, thumb_path, sizeof(thumb_path));

    PS4GroupStateMeta meta;
    memset(&meta, 0, sizeof(meta));
    meta.magic = GROUP_STATE_MAGIC;
    meta.active_count = ctx->active_count;
    meta.single_state_size = (uint32_t)state_size;
    meta.transfer_mode = (int32_t)g_link_hub.transfer_mode;
    meta.cable_enabled = g_link_hub.cable_enabled ? 1 : 0;

    for (int i = 0; i < GBA_MAX_INSTANCES; ++i) {
        if (ctx->instances[i].active && ctx->instances[i].core) {
            meta.active_mask |= (1U << i);
        }
    }

    for (int i = 0; i < LINK_MAX_PLAYERS; ++i) {
        meta.multi_data[i] = g_link_hub.multi_data[i];
        meta.pending_multi_send[i] = g_link_hub.pending_multi_send[i];
        meta.normal8_data[i] = g_link_hub.normal8_data[i];
        meta.normal32_data[i] = g_link_hub.normal32_data[i];
        meta.has_entered_multi[i] = g_link_hub.drivers[i].has_entered_multi ? 1 : 0;
        meta.gpio_grace_frames[i] = g_link_hub.drivers[i].gpio_grace_frames;
        if (g_link_hub.gba[i]) {
            meta.sio_mode[i] = (int32_t)g_link_hub.gba[i]->sio.mode;
            meta.sio_rcnt[i] = g_link_hub.gba[i]->sio.rcnt;
            meta.sio_siocnt[i] = g_link_hub.gba[i]->sio.siocnt;
        }
    }

    for (int i = 0; i < GBA_MAX_INSTANCES; ++i) {
        if (ctx->instances[i].active && ctx->instances[i].core) {
            if (!serialize_core_to_file(ctx->instances[i].core, i, player_paths[i])) {
                return false;
            }
        }
    }

    FILE* meta_fp = fopen(meta_path, "wb");
    if (!meta_fp) {
        return false;
    }
    size_t written = fwrite(&meta, 1, sizeof(meta), meta_fp);
    fflush(meta_fp);
    fclose(meta_fp);

    if (written == sizeof(meta)) {
        capture_and_write_thumbnail(ctx, 0, slot, true, thumb_path, NULL);
    }

    link_hub_sync_pins();
    return (written == sizeof(meta));
}

bool save_manager_group_load_state_slot(struct PS4GBA_Context* ctx, int slot) {
    if (!ctx || ctx->active_count <= 0 || !ctx->instances[0].core) {
        return false;
    }

    ps4_jailbreak_system();
    emulator_pause_workers();

    size_t state_size = ctx->instances[0].core->stateSize(ctx->instances[0].core);
    if (state_size == 0) {
        return false;
    }

    char meta_path[256];
    char player_paths[GBA_MAX_INSTANCES][256];
    build_group_state_paths(ctx->rom_name, slot, meta_path, sizeof(meta_path), player_paths);

    FILE* meta_fp = fopen(meta_path, "rb");
    if (!meta_fp) {
        return false;
    }

    PS4GroupStateMeta meta;
    memset(&meta, 0, sizeof(meta));
    size_t meta_read = fread(&meta, 1, sizeof(meta), meta_fp);
    fclose(meta_fp);

    if (meta_read != sizeof(meta) ||
        meta.magic != GROUP_STATE_MAGIC ||
        meta.single_state_size != (uint32_t)state_size) {
        return false;
    }

    /* Verifica previamente se todos os arquivos .gss necessarios existem antes de alterar a sessao */
    for (int i = 0; i < GBA_MAX_INSTANCES; ++i) {
        if (meta.active_mask & (1U << i)) {
            struct stat st;
            if (stat(player_paths[i], &st) != 0 || (size_t)st.st_size < state_size) {
                return false;
            }
        }
    }

    /* Se o Group Save tinha mais jogadores ativos do que a sessao atual, instancia os slots automaticamente */
    for (int i = 0; i < GBA_MAX_INSTANCES; ++i) {
        if ((meta.active_mask & (1U << i)) && !ctx->instances[i].active) {
            emulator_join_player(i, -1);
        }
    }
    emulator_pause_workers();

    /*
     * 1. Limpa os buffers circulares e eventos pendentes do Link Cable Hub
     *    antes de restaurar as instancias.
     */
    link_hub_reset_buffers();

    /* 2. Carrega os estados de todas as instancias ativas simultaneamente */
    for (int i = 0; i < GBA_MAX_INSTANCES; ++i) {
        if ((meta.active_mask & (1U << i)) && ctx->instances[i].active && ctx->instances[i].core) {
            if (!deserialize_core_from_file(ctx->instances[i].core, i, player_paths[i])) {
                link_hub_reset_buffers();
                return false;
            }
        }
    }

    /* 3. Ressincroniza os registradores e barramentos SIO do Link Cable Hub */
    g_link_hub.transfer_mode = (enum GBASIOMode)meta.transfer_mode;
    g_link_hub.cable_enabled = (meta.cable_enabled != 0);

    for (int i = 0; i < LINK_MAX_PLAYERS; ++i) {
        g_link_hub.multi_data[i] = meta.multi_data[i];
        g_link_hub.pending_multi_send[i] = meta.pending_multi_send[i];
        g_link_hub.normal8_data[i] = meta.normal8_data[i];
        g_link_hub.normal32_data[i] = meta.normal32_data[i];
        g_link_hub.drivers[i].has_entered_multi = (meta.has_entered_multi[i] != 0);
        g_link_hub.drivers[i].gpio_grace_frames = meta.gpio_grace_frames[i];

        struct GBA* g = g_link_hub.gba[i];
        if (g) {
            GBASIOSetDriver(&g->sio, &g_link_hub.drivers[i].d);
            g->sio.mode = (enum GBASIOMode)meta.sio_mode[i];
            g->sio.rcnt = meta.sio_rcnt[i];
            uint16_t scnt = meta.sio_siocnt[i];
            if (g->sio.mode == GBA_SIO_MULTI) {
                scnt = GBASIOMultiplayerClearBusy(scnt);
            } else if (g->sio.mode == GBA_SIO_NORMAL_8 || g->sio.mode == GBA_SIO_NORMAL_32) {
                scnt = GBASIONormalClearStart(scnt);
            }
            g->sio.siocnt = scnt;
            g->memory.io[GBA_REG(RCNT)] = g->sio.rcnt;
            g->memory.io[GBA_REG(SIOCNT)] = g->sio.siocnt;
        }
    }

    link_hub_sync_pins();
    return true;
}

bool save_manager_load_state_preview(struct PS4GBA_Context* ctx, int player_id, int slot,
                                     bool is_group, PS4SaveStatePreview* out_info,
                                     uint32_t* out_argb_pixels) {
    if (!out_info) {
        return false;
    }

    memset(out_info, 0, sizeof(*out_info));
    out_info->slot = slot;
    out_info->player_id = player_id;
    out_info->is_group = is_group;
    out_info->user_id = save_manager_get_active_user_id();
    snprintf(out_info->date_str, sizeof(out_info->date_str), "--/--/----");
    snprintf(out_info->time_str, sizeof(out_info->time_str), "--:--:--");

    if (!ctx) {
        return false;
    }

    ps4_jailbreak_system();

    int safe_player = (player_id >= 0 && player_id < GBA_MAX_INSTANCES) ? player_id : 0;
    out_info->user_id = is_group
                            ? save_manager_get_active_user_id()
                            : (uint32_t)save_manager_get_user_id_for_player(safe_player);
    const char* rname = (!is_group && ctx->instances[safe_player].rom_name[0])
                            ? ctx->instances[safe_player].rom_name
                            : ctx->rom_name;
    strncpy(out_info->rom_title, (rname && *rname) ? rname : "ROM", sizeof(out_info->rom_title) - 1);

    char state_path[256];
    char thumb_path[256];

    if (is_group) {
        char player_paths[GBA_MAX_INSTANCES][256];
        build_group_state_paths(ctx->rom_name, slot, state_path, sizeof(state_path), player_paths);
        build_group_thumb_path(ctx->rom_name, slot, thumb_path, sizeof(thumb_path));
    } else {
        resolve_existing_individual_paths(rname, slot, safe_player,
                                          state_path, sizeof(state_path),
                                          thumb_path, sizeof(thumb_path));
    }

    struct stat st;
    if (stat(state_path, &st) != 0) {
        return false;
    }

    out_info->occupied = true;
    out_info->active_count = (ctx->active_count > 0) ? ctx->active_count : 1;
    format_unix_timestamp((int64_t)st.st_mtime,
                          out_info->date_str, sizeof(out_info->date_str),
                          out_info->time_str, sizeof(out_info->time_str));

    FILE* tf = fopen(thumb_path, "rb");
    if (tf) {
        PS4ThumbnailHeader hdr;
        memset(&hdr, 0, sizeof(hdr));
        if (fread(&hdr, 1, sizeof(hdr), tf) == sizeof(hdr) &&
            hdr.magic == THUMB_FILE_MAGIC &&
            hdr.width == SAVE_THUMB_WIDTH &&
            hdr.height == SAVE_THUMB_HEIGHT) {
            if (hdr.date_str[0] != '\0') {
                strncpy(out_info->date_str, hdr.date_str, sizeof(out_info->date_str) - 1);
            }
            if (hdr.time_str[0] != '\0') {
                strncpy(out_info->time_str, hdr.time_str, sizeof(out_info->time_str) - 1);
            }
            if (hdr.rom_title[0] != '\0') {
                strncpy(out_info->rom_title, hdr.rom_title, sizeof(out_info->rom_title) - 1);
            }
            if (hdr.active_count > 0) {
                out_info->active_count = hdr.active_count;
            }
            if (out_argb_pixels) {
                size_t n = fread(out_argb_pixels, sizeof(uint32_t), SAVE_THUMB_PIXELS, tf);
                if (n == SAVE_THUMB_PIXELS) {
                    out_info->has_thumbnail = true;
                }
            } else {
                out_info->has_thumbnail = true;
            }
        }
        fclose(tf);
    }

    return true;
}

bool save_manager_quick_save_state(struct PS4GBA_Context* ctx, int player_id) {
    return save_manager_quick_save_state_slot(ctx, player_id, s_indiv_slot);
}

bool save_manager_quick_load_state(struct PS4GBA_Context* ctx, int player_id) {
    return save_manager_quick_load_state_slot(ctx, player_id, s_indiv_slot);
}

bool save_manager_group_save_state(struct PS4GBA_Context* ctx) {
    return save_manager_group_save_state_slot(ctx, s_group_slot);
}

bool save_manager_group_load_state(struct PS4GBA_Context* ctx) {
    return save_manager_group_load_state_slot(ctx, s_group_slot);
}
