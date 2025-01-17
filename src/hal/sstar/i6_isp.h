#pragma once

#include "i6_common.h"

typedef struct {
    void *handle, *handleCus3a, *handleIspAlgo;

    int (*fnLoadChannelConfig)(int channel, char *path, unsigned int key);
    int (*fnSetColorToGray)(int channel, char *enable);
} i6_isp_impl;

static int i6_isp_load(i6_isp_impl *isp_lib) {
    if (!(isp_lib->handleIspAlgo = dlopen("libispalgo.so", RTLD_LAZY | RTLD_GLOBAL))) {
        fprintf(stderr, "[i6_isp] Failed to load dependency library!\nError: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    if (!(isp_lib->handleCus3a = dlopen("libcus3a.so", RTLD_LAZY | RTLD_GLOBAL))) {
        fprintf(stderr, "[i6_isp] Failed to load dependency library!\nError: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    if (!(isp_lib->handle = dlopen("libmi_isp.so", RTLD_LAZY | RTLD_GLOBAL))) {
        fprintf(stderr, "[i6_isp] Failed to load library!\nError: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    if (!(isp_lib->fnLoadChannelConfig = (int(*)(int channel, char *path, unsigned int key))
        dlsym(isp_lib->handle, "MI_ISP_API_CmdLoadBinFile"))) {
        fprintf(stderr, "[i6_isp] Failed to acquire symbol MI_ISP_API_CmdLoadBinFile!\nError: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    if (!(isp_lib->fnSetColorToGray = (int(*)(int channel, char *enable))
        dlsym(isp_lib->handle, "MI_ISP_IQ_SetColorToGray"))) {
            fprintf(stderr, "[i6_isp] Failed to acquire symbol MI_ISP_IQ_SetColorToGray!\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void i6_isp_unload(i6_isp_impl *isp_lib) {

    if (isp_lib->handle)
        dlclose(isp_lib->handle = NULL);
    if (isp_lib->handleCus3a)
        dlclose(isp_lib->handleCus3a = NULL);
    if (isp_lib->handleIspAlgo)
        dlclose(isp_lib->handleIspAlgo = NULL);
    memset(isp_lib, 0, sizeof(*isp_lib));
}