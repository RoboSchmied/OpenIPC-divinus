#pragma once

#include "v3_common.h"
#include "v3_isp.h"
#include "v3_snr.h"
#include "v3_vi.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#include "../tools.h"

typedef struct {
    // [sensor]
    char sensor_type[128];
    v3_common_wdr mode;
    char dll_file[256];

    // [mode]
    v3_snr_input input_mode;

    // [mipi]
    v3_snr_mipi mipi;

    // [lvds]
    v3_snr_lvds lvds;

    // [isp_image]
    v3_isp_dev isp;

    // [vi_dev]
    v3_vi_dev videv;

    // [vi_chn]
    v3_vi_chn vichn;
} v3_config_impl;

extern v3_config_impl v3_config;

static enum ConfigError v3_parse_config_lvds(
    struct IniConfig *ini, const char *section, v3_snr_lvds *lvds) {
    enum ConfigError err;
    err = parse_int(
        ini, section, "img_size_w", INT_MIN, INT_MAX, &lvds->capt.width);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "img_size_h", INT_MIN, INT_MAX, &lvds->capt.height);
    if (err != CONFIG_OK)
        return err;
    {
        const char *possible_values[] = {
            "HI_WDR_MODE_NONE",  "HI_WDR_MODE_2F",     "HI_WDR_MODE_3F",
            "HI_WDR_MODE_4F",    "HI_WDR_MODE_DOL_2F", "HI_WDR_MODE_DOL_3F",
            "HI_WDR_MODE_DOL_4F"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "wdr_mode", (void*)&lvds->wdr, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "LVDS_SYNC_MODE_SOL", "LVDS_SYNC_MODE_SAV"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "sync_mode", (void*)&lvds->syncSavOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "RAW_DATA_8BIT", "RAW_DATA_10BIT", "RAW_DATA_12BIT",
            "RAW_DATA_14BIT"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "raw_data_type", (void*)&lvds->prec,
            possible_values, count, 1);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "LVDS_ENDIAN_LITTLE", "LVDS_ENDIAN_BIG"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "data_endian", (void*)&lvds->dataBeOn,
            possible_values, count, 1);
        if (err != CONFIG_OK)
            return err;
        err = parse_enum(
            ini, section, "sync_code_endian", (void*)&lvds->syncBeOn,
            possible_values, count, 1);
        if (err != CONFIG_OK)
            return err;
    }
    err = parse_array(ini, section, "lane_id", lvds->laneId, 8);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "lvds_lane_num", INT_MIN, INT_MAX, &lvds->laneNum);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "wdr_vc_num", INT_MIN, INT_MAX, &lvds->wdrVcNum);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "sync_code_num", INT_MIN, INT_MAX, &lvds->syncCodeNum);
    if (err != CONFIG_OK)
        return err;
    char syncname[16];
    for (int i = 0; i < 8; i++) {
        sprintf(syncname, "sync_code_%d", i);
        err = parse_array(ini, section, syncname, lvds->syncCode[i], 16);
        if (err != CONFIG_OK)
            return err;
    }
    return CONFIG_OK;
}

static enum ConfigError v3_parse_config_videv(
    struct IniConfig *ini, const char *section, v3_vi_dev *device) {
    enum ConfigError err;
    {
        const char *possible_values[] = {
            "VI_INPUT_MODE_BT656",          "VI_INPUT_MODE_BT601",
            "VI_INPUT_MODE_DIGITAL_CAMERA", "VI_INPUT_MODE_INTERLEAVED",
            "VI_INPUT_MODE_MIPI",           "VI_INPUT_MODE_LVDS",
            "VI_INPUT_MODE_HISPI"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "input_mod", (void*)&device->intf,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_WORK_MODE_1Multiplex", "VI_WORK_MODE_2Multiplex",
            "VI_WORK_MODE_4Multiplex"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "work_mod", (void*)&device->work, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_COMBINE_COMPOSITE", "VI_COMBINE_SEPARATE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "combine_mode", (void*)&device->separateOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_COMP_MODE_SINGLE", "VI_COMP_MODE_DOUBLE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "comp_mode", (void*)&device->doubleCompOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_CLK_EDGE_SINGLE_UP", "VI_CLK_EDGE_SINGLE_DOWN",
            "VI_CLK_EDGE_DOUBLE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "clock_edge", (void*)&device->clkDownOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    err = parse_uint32(ini, section, "mask_0", 0, UINT_MAX - 1, &device->cmpntMask[0]);
    if (err != CONFIG_OK)
        return err;
    err = parse_uint32(ini, section, "mask_1", 0, UINT_MAX - 1, &device->cmpntMask[1]);
    if (err != CONFIG_OK)
        return err;
    unsigned int maskNum;
    err = parse_uint32(ini, section, "mask_num", 0, 2, &maskNum);
    if (err != CONFIG_OK)
        return err;
    if (maskNum < 1) device->cmpntMask[1] = UINT_MAX;
    if (maskNum < 2) device->cmpntMask[0] = UINT_MAX;
    {
        const char *possible_values[] = {
            "VI_SCAN_INTERLACED", "VI_SCAN_PROGRESSIVE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "scan_mode", (void*)&device->progressiveOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_INPUT_DATA_VUVU", "VI_INPUT_DATA_UVUV"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "Data_seq", (void*)&device->input, possible_values,
            count, 0);
        if (err != CONFIG_OK) {
            const char *possible_values[] = {
                "VI_INPUT_DATA_UYVY", "VI_INPUT_DATA_VYUY",
                "VI_INPUT_DATA_YUYV", "VI_INPUT_DATA_YVYU"};
            const int count = sizeof(possible_values) / sizeof(const char *);
            err = parse_enum(
                ini, section, "Data_seq", (void*)&device->input,
                possible_values, count, 0);
            if (err != CONFIG_OK)
                return err;
        }
    }
    {
        const char *possible_values[] = {"VI_VSYNC_FIELD", "VI_VSYNC_PULSE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "vsync", (void*)&device->sync.vsyncPulse, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_VSYNC_NEG_HIGH", "VI_VSYNC_NEG_LOW"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "vsyncneg", (void*)&device->sync.vsyncInv,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_HSYNC_VALID_SINGNAL", "VI_HSYNC_PULSE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "hsync", (void*)&device->sync.hsyncPulse, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_HSYNC_NEG_HIGH", "VI_HSYNC_NEG_LOW"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "hsyncneg", (void*)&device->sync.hsyncInv,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_VSYNC_NORM_PULSE", "VI_VSYNC_VALID_SINGAL"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "vsyncvalid", (void*)&device->sync.vsyncValid,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_VSYNC_VALID_NEG_HIGH", "VI_VSYNC_VALID_NEG_LOW"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "vsyncvalidneg", (void*)&device->sync.vsyncValidInv,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    err = parse_int(
        ini, section, "timingblank_hsynchfb", 0, INT_MAX,
        &device->sync.timing.hsyncFront);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_hsyncact", 0, INT_MAX,
        &device->sync.timing.hsyncWidth);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_hsynchbb", 0, INT_MAX,
        &device->sync.timing.hsyncBack);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_vsyncvfb", 0, INT_MAX,
        &device->sync.timing.vsyncFront);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_vsyncvact", 0, INT_MAX,
        &device->sync.timing.vsyncWidth);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_vsyncvbb", 0, INT_MAX,
        &device->sync.timing.vsyncBack);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_vsyncvbfb", 0, INT_MAX,
        &device->sync.timing.vsyncIntrlFront);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_vsyncvbact", 0, INT_MAX,
        &device->sync.timing.vsyncIntrlWidth);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "timingblank_vsyncvbbb", 0, INT_MAX,
        &device->sync.timing.vsyncIntrlBack);
    if (err != CONFIG_OK)
        return err;
    {
        const char *possible_values[] = {"BT656_FIXCODE_1", "BT656_FIXCODE_0"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "fixcode", (void*)&device->codeZeroOn, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "BT656_FIELD_POLAR_STD", "BT656_FIELD_POLAR_NSTD"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "fieldpolar", (void*)&device->polarNstdOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_PATH_BYPASS", "VI_PATH_ISP", "VI_PATH_RAW"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "datapath", (void*)&device->dataPath,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "VI_DATA_TYPE_YUV", "VI_DATA_TYPE_RGB"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "inputdatatype", &device->rgbModeOn,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    err = parse_int(ini, section, "datarev", 0, INT_MAX, &device->dataRev);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, section, "devrect_x", 0, INT_MAX, &device->capt.x);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, section, "devrect_y", 0, INT_MAX, &device->capt.y);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, section, "devrect_w", 0, INT_MAX, &device->capt.width);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, section, "devrect_h", 0, INT_MAX, &device->capt.height);
    if (err != CONFIG_OK)
        return err;
    return CONFIG_OK;
}

static enum ConfigError v3_parse_config_vichn(
    struct IniConfig *ini, const char *section, v3_vi_chn *channel) {
    enum ConfigError err;
    err = parse_int(ini, section, "caprect_x", 0, INT_MAX, &channel->capt.x);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, section, "caprect_y", 0, INT_MAX, &channel->capt.y);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "caprect_width", 0, INT_MAX, &channel->capt.width);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "caprect_height", 0, INT_MAX, &channel->capt.height);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "destsize_width", 0, INT_MAX, &channel->dest.width);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "destsize_height", 0, INT_MAX, &channel->dest.height);
    if (err != CONFIG_OK)
        return err;
    {
        const char *possible_values[] = {
            "VI_CAPSEL_TOP", "VI_CAPSEL_BOTTOM", "VI_CAPSEL_BOTH"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "capsel", (void*)&channel->field, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "PIXEL_FORMAT_RGB_1BPP",
            "PIXEL_FORMAT_RGB_2BPP",
            "PIXEL_FORMAT_RGB_4BPP",
            "PIXEL_FORMAT_RGB_8BPP",
            "PIXEL_FORMAT_RGB_444",
            "PIXEL_FORMAT_RGB_4444",
            "PIXEL_FORMAT_RGB_555",
            "PIXEL_FORMAT_RGB_565",
            "PIXEL_FORMAT_RGB_1555",
            "PIXEL_FORMAT_RGB_888",
            "PIXEL_FORMAT_RGB_8888",
            "PIXEL_FORMAT_RGB_PLANAR_888",
            "PIXEL_FORMAT_RGB_BAYER_8BPP",
            "PIXEL_FORMAT_RGB_BAYER_10BPP",
            "PIXEL_FORMAT_RGB_BAYER_12BPP",
            "PIXEL_FORMAT_RGB_BAYER_14BPP",
            "PIXEL_FORMAT_RGB_BAYER",
            "PIXEL_FORMAT_YUV_A422",
            "PIXEL_FORMAT_YUV_A444",
            "PIXEL_FORMAT_YUV_PLANAR_422",
            "PIXEL_FORMAT_YUV_PLANAR_420",
            "PIXEL_FORMAT_YUV_PLANAR_444",
            "PIXEL_FORMAT_YUV_SEMIPLANAR_422",
            "PIXEL_FORMAT_YUV_SEMIPLANAR_420",
            "PIXEL_FORMAT_YUV_SEMIPLANAR_444",
            "PIXEL_FORMAT_UYVY_PACKAGE_422",
            "PIXEL_FORMAT_YUYV_PACKAGE_422",
            "PIXEL_FORMAT_VYUY_PACKAGE_422",
            "PIXEL_FORMAT_YCbCr_PLANAR",
            "PIXEL_FORMAT_YUV_400"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "pixformat", (void*)&channel->pixFmt,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    {
        const char *possible_values[] = {
            "COMPRESS_MODE_NONE", "COMPRESS_MODE_SEG", "COMPRESS_MODE_SEG128",
            "COMPRESS_MODE_LINE", "COMPRESS_MODE_FRAME"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, section, "compressmode", (void*)&channel->compress,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    err = parse_int(
        ini, section, "srcframeRate", INT_MIN, INT_MAX, &channel->srcFps);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, section, "framerate", INT_MIN, INT_MAX, &channel->dstFps);
    if (err != CONFIG_OK)
        return err;
    return CONFIG_OK;
}

static enum ConfigError v3_parse_config_isp(
    struct IniConfig *ini, const char *section, v3_isp_dev *isp) {
    enum ConfigError err;
    err = parse_int(ini, "isp_image", "isp_x", 0, INT_MAX, &isp->capt.x);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, "isp_image", "isp_y", 0, INT_MAX, &isp->capt.y);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, "isp_image", "isp_w", 0, INT_MAX, &isp->capt.width);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(ini, "isp_image", "isp_h", 0, INT_MAX, &isp->capt.height);
    if (err != CONFIG_OK)
        return err;
    err = parse_int(
        ini, "isp_image", "isp_frameRate", 0, INT_MAX, &isp->framerate);
    if (err != CONFIG_OK)
        return err;
    {
        const char *possible_values[] = {
            "BAYER_RGGB", "BAYER_GRBG", "BAYER_GBRG", "BAYER_BGGR"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            ini, "isp_image", "isp_bayer", (void*)&isp->bayer,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            return err;
    }
    return CONFIG_OK;
}

static enum ConfigError v3_parse_sensor_config(char *path, v3_config_impl *config) {
    if (config == NULL)
        return (enum ConfigError)-1;
    memset(config, 0, sizeof(config));
    struct IniConfig ini;
    memset(&ini, 0, sizeof(ini));
    enum ConfigError err;

    // load config file to string
    ini.str = NULL;
    {
        FILE *file = fopen(path, "rb");
        if (!file) {
            printf("Can't open file %s\n", path);
            return (enum ConfigError)-1;
        }

        fseek(file, 0, SEEK_END);
        size_t length = (size_t)ftell(file);
        fseek(file, 0, SEEK_SET);

        ini.str = (char*)malloc(length + 1);
        if (!ini.str) {
            printf("Can't allocate buf in parse_sensor_config\n");
            fclose(file);
            return (enum ConfigError)-1;
        }
        size_t n = fread(ini.str, 1, length, file);
        if (n != length) {
            printf("Can't read all file %s\n", path);
            fclose(file);
            free(ini.str);
            return (enum ConfigError)-1;
        }
        fclose(file);
        ini.str[length] = 0;
    }

    find_sections(&ini);

    // [sensor]
    err = parse_param_value(&ini, "sensor", "sensor_type", config->sensor_type);
    if (err != CONFIG_OK)
        goto RET_ERR;
    {
        const char *possible_values[] = {
            "WDR_MODE_NONE",
            "WDR_MODE_BUILT_IN",
            "WDR_MODE_2To1_LINE",
            "WDR_MODE_2To1_FRAME",
            "WDR_MODE_2To1_FRAME_FULL_RATE",
            "WDR_MODE_3To1_LINE",
            "WDR_MODE_3To1_FRAME",
            "WDR_MODE_3To1_FRAME_FULL_RATE",
            "WDR_MODE_4To1_LINE",
            "WDR_MODE_4To1_FRAME",
            "WDR_MODE_4To1_FRAME_FULL_RATE"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            &ini, "sensor", "mode", (void*)&config->mode, possible_values,
            count, 0);
        if (err != CONFIG_OK)
            goto RET_ERR;
    }
    err = parse_param_value(&ini, "sensor", "dllfile", config->dll_file);
    if (err != CONFIG_OK)
        goto RET_ERR;

    // [mode]
    {
        const char *possible_values[] = {
            "INPUT_MODE_MIPI",   "INPUT_MODE_SUBLVDS",  "INPUT_MODE_LVDS",
            "INPUT_MODE_HISPI",  "INPUT_MODE_CMOS_18V", "INPUT_MODE_CMOS_33V",
            "INPUT_MODE_BT1120", "INPUT_MODE_BYPASS"};
        const int count = sizeof(possible_values) / sizeof(const char *);
        err = parse_enum(
            &ini, "mode", "input_mode", (void*)&config->input_mode,
            possible_values, count, 0);
        if (err != CONFIG_OK)
            goto RET_ERR;
    }

    if (config->input_mode == V3_VI_INTF_MIPI) {
        // [mipi]
        {
            const char *possible_values[] = {// starts from 1 !!!!!
                                             "RAW_DATA_8BIT", "RAW_DATA_10BIT",
                                             "RAW_DATA_12BIT",
                                             "RAW_DATA_14BIT"};
            const int count = sizeof(possible_values) / sizeof(const char *);
            err = parse_enum(
                &ini, "mipi", "data_type", (void*)&config->mipi.prec,
                possible_values, count, 1);
            if (err != CONFIG_OK)
                goto RET_ERR;
        }
        err = parse_array(&ini, "mipi", "lane_id", (int*)&config->mipi.laneId, 8);
        if (err != CONFIG_OK)
            goto RET_ERR;
    } else if (config->input_mode == V3_VI_INTF_LVDS) {
        // [lvds]
        err = v3_parse_config_lvds(&ini, "lvds", &config->lvds);
        if (err != CONFIG_OK)
            goto RET_ERR;
    }

    // [isp_image]
    err = v3_parse_config_isp(&ini, "ips_image", &config->isp);
    if (err != CONFIG_OK)
        goto RET_ERR;
    // [vi_dev]
    err = v3_parse_config_videv(&ini, "vi_dev", &config->videv);
    if (err != CONFIG_OK)
        goto RET_ERR;
    // [vi_chn]
    err = v3_parse_config_vichn(&ini, "vi_chn", &config->vichn);
    if (err != CONFIG_OK)
        goto RET_ERR;

    v3_config = *config;
    free(ini.str);
    return CONFIG_OK;
RET_ERR:
    free(ini.str);
    return err;
}