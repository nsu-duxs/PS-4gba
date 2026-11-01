#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <wchar.h>
#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
#include "emulator_manager.h"
#include "video_split.h"
#include "input_ds4.h"
#include "audio_ps4.h"
#include "ui_menu.h"
#include "ps4_jailbreak.h"

/* ============================================================================
 * 1. Overrides de funcoes da libc (musl) que acessam %fs:[0] (TLS) ou ENOSYS
 * ============================================================================ */
char* strerror(int errnum) {
    (void)errnum;
    return (char*)"Orbis error";
}

size_t wcrtomb(char* restrict s, wchar_t wc, mbstate_t* restrict st) {
    (void)st;
    if (!s) {
        return 1;
    }
    *s = (char)(wc & 0xFF);
    return 1;
}

int __lockfile(FILE* f) {
    (void)f;
    return 0;
}

void __unlockfile(FILE* f) {
    (void)f;
}

char* getcwd(char* buf, size_t size) {
    static const char k_default_cwd[] = "/data/gba4p";
    if (!buf || size == 0) {
        return NULL;
    }
    if (size < sizeof(k_default_cwd)) {
        buf[0] = '\0';
        return NULL;
    }
    memcpy(buf, k_default_cwd, sizeof(k_default_cwd));
    return buf;
}

struct tm* __localtime_r(const time_t* restrict t, struct tm* restrict tm_out) {
    if (!tm_out) {
        return NULL;
    }
    memset(tm_out, 0, sizeof(*tm_out));
    time_t sec = t ? *t : 0;
    if (sec < 0) {
        sec = 0;
    }

    int64_t days = (int64_t)(sec / 86400);
    int32_t rem = (int32_t)(sec % 86400);

    tm_out->tm_hour = rem / 3600;
    rem %= 3600;
    tm_out->tm_min = rem / 60;
    tm_out->tm_sec = rem % 60;
    tm_out->tm_wday = (int)((days + 4) % 7);

    int64_t z = days + 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t y = (int64_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t d = doy - (153 * mp + 2) / 5 + 1;
    uint32_t m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);

    tm_out->tm_year = (int)(y - 1900);
    tm_out->tm_mon = (int)(m - 1);
    tm_out->tm_mday = (int)d;
    tm_out->tm_yday = (int)doy;
    return tm_out;
}

struct tm* localtime_r(const time_t* restrict t, struct tm* restrict tm_out) {
    return __localtime_r(t, tm_out);
}

/* ============================================================================
 * 2. Alocador de Heap via PS4 Direct Memory (512 MB)
 *    Evita o limite de 64 MB de MAP_ANON, mremap(ENOSYS) e hlt(0xf4) da musl
 * ============================================================================ */
#define ORBIS_HEAP_MAGIC 0x4F424850U

typedef struct __attribute__((aligned(64))) OrbisHeapBlock {
    size_t size;
    struct OrbisHeapBlock* next;
    struct OrbisHeapBlock* prev;
    uint32_t is_free;
    uint32_t magic;
} OrbisHeapBlock;

static OrbisHeapBlock* s_heap_head = NULL;
static OrbisHeapBlock* s_heap_rover = NULL;
static uintptr_t s_heap_start = 0;
static uintptr_t s_heap_end = 0;
static off_t s_heap_phys_addr = 0;
static size_t s_heap_pool_size = 0;
static bool s_heap_terminated = false;
static volatile unsigned char s_heap_lock = 0;

static inline void heap_lock(void) {
    while (__atomic_test_and_set(&s_heap_lock, __ATOMIC_ACQUIRE)) {
#if defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#endif
    }
}

static inline void heap_unlock(void) {
    __atomic_clear(&s_heap_lock, __ATOMIC_RELEASE);
}

static bool heap_ensure_initialized(void) {
    if (s_heap_terminated) {
        return false;
    }
    if (s_heap_head) {
        return true;
    }

    size_t dmem_total = sceKernelGetDirectMemorySize();
    if (dmem_total == 0) {
        dmem_total = 1024ULL * 1024ULL * 1024ULL;
    }

    static const size_t k_try_sizes[] = {
        512ULL * 1024ULL * 1024ULL,
        256ULL * 1024ULL * 1024ULL,
        128ULL * 1024ULL * 1024ULL
    };

    for (size_t i = 0; i < sizeof(k_try_sizes) / sizeof(k_try_sizes[0]); ++i) {
        size_t pool_size = k_try_sizes[i];
        off_t phys_addr = 0;
        if (sceKernelAllocateDirectMemory(0, (off_t)dmem_total, pool_size, 16384, 0, &phys_addr) == 0) {
            void* mapped = NULL;
            if (sceKernelMapDirectMemory(&mapped, pool_size, 0x03, 0, phys_addr, 16384) == 0 && mapped) {
                s_heap_phys_addr = phys_addr;
                s_heap_pool_size = pool_size;
                s_heap_start = (uintptr_t)mapped;
                s_heap_end = s_heap_start + pool_size;
                s_heap_head = (OrbisHeapBlock*)mapped;
                s_heap_head->size = pool_size - sizeof(OrbisHeapBlock);
                s_heap_head->next = NULL;
                s_heap_head->prev = NULL;
                s_heap_head->is_free = 1;
                s_heap_head->magic = ORBIS_HEAP_MAGIC;
                s_heap_rover = s_heap_head;
                return true;
            }
            sceKernelReleaseDirectMemory(phys_addr, pool_size);
        }
    }

    return false;
}

static void heap_shutdown(void) {
    heap_lock();
    s_heap_terminated = true;
    void* mapped = (void*)s_heap_start;
    size_t pool_size = s_heap_pool_size;
    off_t phys_addr = s_heap_phys_addr;

    s_heap_head = NULL;
    s_heap_rover = NULL;
    s_heap_start = 0;
    s_heap_end = 0;
    s_heap_phys_addr = 0;
    s_heap_pool_size = 0;
    heap_unlock();

    if (mapped && pool_size > 0) {
        sceKernelMunmap(mapped, pool_size);
    }
    if (phys_addr != 0 && pool_size > 0) {
        sceKernelReleaseDirectMemory(phys_addr, pool_size);
    }
}

static void* heap_alloc_unlocked(size_t size) {
    if (!heap_ensure_initialized()) {
        return NULL;
    }

    size_t aligned = (size == 0) ? 64U : ((size + 63U) & ~((size_t)63U));
    OrbisHeapBlock* start = s_heap_rover ? s_heap_rover : s_heap_head;
    OrbisHeapBlock* cur = start;

    for (int pass = 0; pass < 2; ++pass) {
        for (; cur != NULL; cur = cur->next) {
            if (pass == 1 && cur == start) {
                break;
            }
            if (cur->is_free && cur->size >= aligned) {
                if (cur->size >= aligned + sizeof(OrbisHeapBlock) + 64U) {
                    OrbisHeapBlock* split = (OrbisHeapBlock*)((uint8_t*)(cur + 1) + aligned);
                    split->size = cur->size - aligned - sizeof(OrbisHeapBlock);
                    split->is_free = 1;
                    split->magic = ORBIS_HEAP_MAGIC;
                    split->next = cur->next;
                    split->prev = cur;
                    if (split->next) {
                        split->next->prev = split;
                    }
                    cur->next = split;
                    cur->size = aligned;
                    s_heap_rover = split;
                } else {
                    s_heap_rover = cur->next ? cur->next : s_heap_head;
                }
                cur->is_free = 0;
                return (void*)(cur + 1);
            }
        }
        cur = s_heap_head;
    }
    return NULL;
}

static void heap_free_unlocked(void* ptr) {
    if (!ptr) {
        return;
    }
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < s_heap_start + sizeof(OrbisHeapBlock) || addr >= s_heap_end) {
        return;
    }

    OrbisHeapBlock* block = ((OrbisHeapBlock*)ptr) - 1;
    if (block->magic != ORBIS_HEAP_MAGIC || block->is_free) {
        return;
    }

    block->is_free = 1;

    if (block->next && block->next->is_free) {
        OrbisHeapBlock* n = block->next;
        if (s_heap_rover == n) {
            s_heap_rover = block;
        }
        block->size += sizeof(OrbisHeapBlock) + n->size;
        block->next = n->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->is_free) {
        OrbisHeapBlock* p = block->prev;
        if (s_heap_rover == block) {
            s_heap_rover = p;
        }
        p->size += sizeof(OrbisHeapBlock) + block->size;
        p->next = block->next;
        if (p->next) {
            p->next->prev = p;
        }
        block = p;
    }

    s_heap_rover = block;
}

void* malloc(size_t size) {
    heap_lock();
    void* ptr = heap_alloc_unlocked(size);
    heap_unlock();
    return ptr;
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }
    heap_lock();
    heap_free_unlocked(ptr);
    heap_unlock();
}

void* calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > ((size_t)-1) / nmemb) {
        return NULL;
    }
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    uintptr_t addr = (uintptr_t)ptr;
    if (addr < s_heap_start + sizeof(OrbisHeapBlock) || addr >= s_heap_end) {
        return NULL;
    }

    heap_lock();
    OrbisHeapBlock* block = ((OrbisHeapBlock*)ptr) - 1;
    if (block->magic != ORBIS_HEAP_MAGIC) {
        heap_unlock();
        return NULL;
    }

    size_t old_size = block->size;
    size_t aligned = (size + 63U) & ~((size_t)63U);
    if (old_size >= aligned) {
        heap_unlock();
        return ptr;
    }

    if (block->next && block->next->is_free &&
        (old_size + sizeof(OrbisHeapBlock) + block->next->size) >= aligned) {
        OrbisHeapBlock* n = block->next;
        if (s_heap_rover == n) {
            s_heap_rover = block;
        }
        block->size += sizeof(OrbisHeapBlock) + n->size;
        block->next = n->next;
        if (block->next) {
            block->next->prev = block;
        }
        if (block->size >= aligned + sizeof(OrbisHeapBlock) + 64U) {
            OrbisHeapBlock* split = (OrbisHeapBlock*)((uint8_t*)(block + 1) + aligned);
            split->size = block->size - aligned - sizeof(OrbisHeapBlock);
            split->is_free = 1;
            split->magic = ORBIS_HEAP_MAGIC;
            split->next = block->next;
            split->prev = block;
            if (split->next) {
                split->next->prev = split;
            }
            block->next = split;
            block->size = aligned;
            s_heap_rover = split;
        }
        heap_unlock();
        return ptr;
    }

    void* new_ptr = heap_alloc_unlocked(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size);
        heap_free_unlocked(ptr);
    }
    heap_unlock();
    return new_ptr;
}

/* ============================================================================
 * 3. Gestao de Sinais POSIX e Sequencia Deterministica de Teardown
 * ============================================================================ */
static volatile sig_atomic_t s_quit_requested = 0;
static volatile unsigned char s_teardown_executed = 0;

static void posix_signal_handler(int sig) {
    (void)sig;
    s_quit_requested = 1;
}

static void app_teardown_sequence(void) {
    if (__atomic_test_and_set(&s_teardown_executed, __ATOMIC_ACQ_REL)) {
        return;
    }

    /* 1. Encerra threads de emulacao (Cores 1..3), persiste SRAM em disco e libera instancias mGBA */
    emulator_destroy_all();

    /* 2. Sinaliza e aguarda thread de audio de baixa latencia (Core 4) e fecha porta SceAudioOut */
    audio_shutdown();

    /* 3. Restaura lightbar, fecha descritores Bluetooth HID (scePad) e encerra SceUserService */
    input_shutdown();

    /* 4. Aguarda ultimo VSync flip (DCE), desregistra buffers, fecha SceVideoOut e libera Direct Memory de video */
    video_shutdown();

    /* 5. Desmonta nullfs USB do sandbox e restaura credenciais/vnodes originais do kernel FreeBSD */
    ps4_unjailbreak_system();

    /* 6. Desmapeia e libera o pool de Direct Memory do heap customizado */
    heap_shutdown();
}

int main(int argc, char* argv[]) {
    /* Fixa a thread principal (Player 0) exclusivamente no Nucleo 0 do Cluster 0 Jaguar */
    typedef OrbisPthread (*ScePthreadSelfFn)(void);
    OrbisPthread main_thread = ((ScePthreadSelfFn)(uintptr_t)scePthreadSelf)();
    scePthreadSetaffinity(main_thread, 1ULL << 0);
    ps4_jailbreak_system();
    (void)argc;
    (void)argv;

    signal(SIGINT, posix_signal_handler);
    signal(SIGTERM, posix_signal_handler);
    signal(SIGQUIT, posix_signal_handler);
    signal(SIGHUP, posix_signal_handler);
    atexit(app_teardown_sequence);

    if (!video_init()) {
        app_teardown_sequence();
        return -1;
    }

    input_init();
    audio_init();
    ui_menu_init(); /* Inicia no estado animado UI_STATE_SPLASH */

    while (!s_quit_requested && !ui_menu_should_quit()) {
        input_poll_all(&g_gba_ctx);

        if (s_quit_requested || ui_menu_should_quit()) {
            break;
        }

        /* Emula apenas durante o jogo ativo; Splash e Menus renderizam independentes a 60 FPS */
        if (ui_menu_get_state() == UI_STATE_INGAME) {
            emulator_step_lockstep_frame();
            audio_process(&g_gba_ctx);
        }

        video_render_split_screen(&g_gba_ctx);
    }

    app_teardown_sequence();
    sceSystemServiceLoadExec("exit", NULL);

    return 0;
}
