#include "ps4_jailbreak.h"
#include "libjbc/libjbc.h"
#include <stdio.h>
#include <string.h>

static bool s_jailbroken = false;
static bool s_has_orig_cred = false;
static jbc_cred s_orig_cred;
static bool s_usb_mounted[8] = { false };

bool ps4_jailbreak_system(void) {
    if (!s_jailbroken) {
        jbc_cred cred;
        memset(&cred, 0, sizeof(cred));
        if (jbc_get_cred(&cred) == 0) {
            if (!s_has_orig_cred) {
                s_orig_cred = cred;
                s_has_orig_cred = true;
            }
            if (jbc_jailbreak_cred(&cred) == 0) {
                if (jbc_set_cred(&cred) == 0) {
                    s_jailbroken = true;
                }
            }
        }
    }

    /* Tenta mapear pontos de montagem no sandbox apenas se ainda nao montados */
    for (int u = 0; u < 8; ++u) {
        if (!s_usb_mounted[u]) {
            char sys_usb[32];
            char mnt_usb[16];
            snprintf(sys_usb, sizeof(sys_usb), "/mnt/usb%d", u);
            snprintf(mnt_usb, sizeof(mnt_usb), "usb%d", u);
            if (jbc_mount_in_sandbox(sys_usb, mnt_usb) == 0) {
                s_usb_mounted[u] = true;
            }
        }
    }

    return s_jailbroken;
}

void ps4_unjailbreak_system(void) {
    for (int u = 0; u < 8; ++u) {
        if (s_usb_mounted[u]) {
            char mnt_usb[16];
            snprintf(mnt_usb, sizeof(mnt_usb), "usb%d", u);
            jbc_unmount_in_sandbox(mnt_usb);
            s_usb_mounted[u] = false;
        }
    }

    if (s_jailbroken && s_has_orig_cred) {
        jbc_set_cred(&s_orig_cred);
        s_jailbroken = false;
    }
}

bool ps4_is_jailbroken(void) {
    return s_jailbroken;
}
