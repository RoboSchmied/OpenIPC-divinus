#include "i6f_hal.h"

i6f_isp_impl  i6f_isp;
i6f_rgn_impl  i6f_rgn;
i6f_scl_impl  i6f_scl;
i6f_snr_impl  i6f_snr;
i6f_sys_impl  i6f_sys;
i6f_venc_impl i6f_venc;
i6f_vif_impl  i6f_vif;

hal_chnstate i6f_state[I6F_VENC_CHN_NUM] = {0};
int (*i6f_venc_cb)(char, hal_vidstream*);

i6f_snr_pad _i6f_snr_pad;
i6f_snr_plane _i6f_snr_plane;
char _i6f_snr_framerate, _i6f_snr_hdr, _i6f_snr_index, _i6f_snr_profile;

char _i6f_isp_chn = 0;
char _i6f_isp_dev = 0;
char _i6f_isp_port = 0;
char _i6f_scl_chn = 0;
char _i6f_scl_dev = 0;
char _i6f_venc_port = 0;
char _i6f_vif_chn = 0;
char _i6f_vif_dev = 0;
char _i6f_vif_grp = 0;

void i6f_hal_deinit(void)
{
    i6f_vif_unload(&i6f_vif);
    i6f_venc_unload(&i6f_venc);
    i6f_snr_unload(&i6f_snr);
    i6f_scl_unload(&i6f_scl);
    i6f_rgn_unload(&i6f_rgn);
    i6f_isp_unload(&i6f_isp);
    i6f_sys_unload(&i6f_sys);
}

int i6f_hal_init(void)
{
    int ret;

    if (ret = i6f_sys_load(&i6f_sys))
        return ret;
    if (ret = i6f_isp_load(&i6f_isp))
        return ret;
    if (ret = i6f_rgn_load(&i6f_rgn))
        return ret;
    if (ret = i6f_scl_load(&i6f_scl))
        return ret;
    if (ret = i6f_snr_load(&i6f_snr))
        return ret;
    if (ret = i6f_venc_load(&i6f_venc))
        return ret;
    if (ret = i6f_vif_load(&i6f_vif))
        return ret;

    return EXIT_SUCCESS;
}

int i6f_channel_bind(char index, char framerate, char jpeg)
{
    int ret;

    if (ret = i6f_scl.fnEnablePort(_i6f_scl_dev, _i6f_scl_chn, index))
        return ret;

    {
        i6f_sys_bind source = { .module = I6F_SYS_MOD_SCL, 
            .device = _i6f_scl_dev, .channel = _i6f_scl_chn, .port = index };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_VENC,
            .device = jpeg ? I6F_VENC_DEV_MJPG_0 : I6F_VENC_DEV_H26X_0, 
            .channel = index, .port = _i6f_venc_port };
        if (ret = i6f_sys.fnBindExt(0, &source, &dest, framerate, framerate,
            I6F_SYS_LINK_REALTIME, 0))
            return ret;
    }

    return EXIT_SUCCESS;
}

int i6f_channel_create(char index, short width, short height, char mirror, char flip, char jpeg)
{
    i6f_scl_port port;
    port.crop.x = 0;
    port.crop.y = 0;
    port.crop.width = 0;
    port.crop.height = 0;
    port.output.width = width;
    port.output.height = height;
    port.mirror = mirror;
    port.flip = flip;
    port.compress = I6F_COMPR_NONE;
    port.pixFmt = jpeg ? I6F_PIXFMT_YUV422_YUYV : I6F_PIXFMT_YUV420SP;

    return i6f_scl.fnSetPortConfig(_i6f_scl_dev, _i6f_scl_chn, index, &port);
}

int i6f_channel_grayscale(char enable)
{
    return i6f_isp.fnSetColorToGray(_i6f_isp_dev, 0, &enable);
}

int i6f_channel_unbind(char index, char jpeg)
{
    int ret;

    if (ret = i6f_scl.fnDisablePort(_i6f_scl_dev, _i6f_scl_chn, index))
        return ret;

    {
        i6f_sys_bind source = { .module = I6F_SYS_MOD_SCL, 
            .device = _i6f_scl_dev, .channel = _i6f_scl_chn, .port = index };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_VENC,
            .device = jpeg ? I6F_VENC_DEV_MJPG_0 : I6F_VENC_DEV_H26X_0, .channel = index, .port = _i6f_venc_port };
        if (ret = i6f_sys.fnUnbind(0, &source, &dest))
            return ret;
    }

    return EXIT_SUCCESS;    
}

int i6f_config_load(char *path)
{
    return i6f_isp.fnLoadChannelConfig(_i6f_isp_dev, _i6f_isp_chn, path, 1234);
}

int i6f_encoder_create(char index, hal_vidconfig *config)
{
    int ret;
    char device = I6F_VENC_DEV_H26X_0;
    i6f_venc_chn channel;
    i6f_venc_attr_h26x *attrib;
    
    if (config->codec == HAL_VIDCODEC_JPG || config->codec == HAL_VIDCODEC_MJPG) {
        device = I6F_VENC_DEV_MJPG_0;
        channel.attrib.codec = I6F_VENC_CODEC_MJPG;
        switch (config->mode) {
            case HAL_VIDMODE_CBR:
                channel.rate.mode = I6F_VENC_RATEMODE_MJPGCBR;
                channel.rate.mjpgCbr.bitrate = config->bitrate << 10;
                channel.rate.mjpgCbr.fpsNum = 
                    config->codec == HAL_VIDCODEC_JPG ? 1 : config->framerate;
                channel.rate.mjpgCbr.fpsDen = 1;
                break;
            case HAL_VIDMODE_QP:
                channel.rate.mode = I6F_VENC_RATEMODE_MJPGQP;
                channel.rate.mjpgQp.fpsNum = config->framerate;
                channel.rate.mjpgQp.fpsDen = 
                    config->codec == HAL_VIDCODEC_JPG ? 1 : config->framerate;
                channel.rate.mjpgQp.quality = MAX(config->minQual, config->maxQual);
                break;
            default:
                I6F_ERROR("MJPEG encoder can only support CBR or fixed QP modes!");
        }
        channel.attrib.mjpg.maxHeight = ALIGN_BACK(config->height, 16);
        channel.attrib.mjpg.maxWidth = ALIGN_BACK(config->width, 16);
        channel.attrib.mjpg.bufSize = config->width * config->height;
        channel.attrib.mjpg.byFrame = 1;
        channel.attrib.mjpg.height = ALIGN_BACK(config->height, 16);
        channel.attrib.mjpg.width = ALIGN_BACK(config->width, 16);
        channel.attrib.mjpg.dcfThumbs = 0;
        channel.attrib.mjpg.markPerRow = 0;

        goto attach;
    } else if (config->codec == HAL_VIDCODEC_H265) {
        channel.attrib.codec = I6F_VENC_CODEC_H265;
        attrib = &channel.attrib.h265;
        switch (config->mode) {
            case HAL_VIDMODE_CBR:
                channel.rate.mode = I6F_VENC_RATEMODE_H265CBR;
                channel.rate.h265Cbr = (i6f_venc_rate_h26xcbr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1, .bitrate = 
                    (unsigned int)(config->bitrate) << 10, .avgLvl = 1 }; break;
            case HAL_VIDMODE_VBR:
                channel.rate.mode = I6F_VENC_RATEMODE_H265VBR;
                channel.rate.h265Vbr = (i6f_venc_rate_h26xvbr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1, .maxBitrate = 
                    (unsigned int)(MAX(config->bitrate, config->maxBitrate)) << 10,
                    .maxQual = config->maxQual, .minQual = config->minQual }; break;
            case HAL_VIDMODE_QP:
                channel.rate.mode = I6F_VENC_RATEMODE_H265QP;
                channel.rate.h265Qp = (i6f_venc_rate_h26xqp){ .gop = config->gop,
                    .fpsNum =  config->framerate, .fpsDen = 1, .interQual = config->maxQual,
                    .predQual = config->minQual }; break;
            case HAL_VIDMODE_ABR:
                I6F_ERROR("H.265 encoder does not support ABR mode!");
            case HAL_VIDMODE_AVBR:
                channel.rate.mode = I6F_VENC_RATEMODE_H265AVBR;
                channel.rate.h265Avbr = (i6f_venc_rate_h26xvbr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1, .maxBitrate = 
                    (unsigned int)(MAX(config->bitrate, config->maxBitrate)) << 10,
                    .maxQual = config->maxQual, .minQual = config->minQual }; break;
            default:
                I6F_ERROR("H.265 encoder does not support this mode!");
        }  
    } else if (config->codec == HAL_VIDCODEC_H264) {
        channel.attrib.codec = I6F_VENC_CODEC_H264;
        attrib = &channel.attrib.h264;
        switch (config->mode) {
            case HAL_VIDMODE_CBR:
                channel.rate.mode = I6F_VENC_RATEMODE_H264CBR;
                channel.rate.h264Cbr = (i6f_venc_rate_h26xcbr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1, .bitrate = 
                    (unsigned int)(config->bitrate) << 10, .avgLvl = 1 }; break;
            case HAL_VIDMODE_VBR:
                channel.rate.mode = I6F_VENC_RATEMODE_H264VBR;
                channel.rate.h264Vbr = (i6f_venc_rate_h26xvbr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1, .maxBitrate = 
                    (unsigned int)(MAX(config->bitrate, config->maxBitrate)) << 10,
                    .maxQual = config->maxQual, .minQual = config->minQual }; break;
            case HAL_VIDMODE_QP:
                channel.rate.mode = I6F_VENC_RATEMODE_H264QP;
                channel.rate.h264Qp = (i6f_venc_rate_h26xqp){ .gop = config->gop,
                    .fpsNum = config->framerate, .fpsDen = 1, .interQual = config->maxQual,
                    .predQual = config->minQual }; break;
            case HAL_VIDMODE_ABR:
                channel.rate.mode = I6F_VENC_RATEMODE_H264ABR;
                channel.rate.h264Abr = (i6f_venc_rate_h26xabr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1,
                    .avgBitrate = (unsigned int)(config->bitrate) << 10,
                    .maxBitrate = (unsigned int)(config->maxBitrate) << 10 }; break;
            case HAL_VIDMODE_AVBR:
                channel.rate.mode = I6F_VENC_RATEMODE_H265AVBR;
                channel.rate.h265Avbr = (i6f_venc_rate_h26xvbr){ .gop = config->gop,
                    .statTime = 1, .fpsNum = config->framerate, .fpsDen = 1, .maxBitrate = 
                    (unsigned int)(MAX(config->bitrate, config->maxBitrate)) << 10,
                    .maxQual = config->maxQual, .minQual = config->minQual }; break;
            default:
                I6F_ERROR("H.264 encoder does not support this mode!");
        }
    } else I6F_ERROR("This codec is not supported by the hardware!");
    attrib->maxHeight = ALIGN_BACK(config->height, 16);
    attrib->maxWidth = ALIGN_BACK(config->width, 16);
    attrib->bufSize = config->height * config->width;
    attrib->profile = config->profile;
    attrib->byFrame = 1;
    attrib->height = ALIGN_BACK(config->height, 16);
    attrib->width = ALIGN_BACK(config->width, 16);
    attrib->bFrameNum = 0;
    attrib->refNum = 1;
attach:
    if (ret = i6f_venc.fnCreateChannel(device, index, &channel))
        return ret;

    if (config->codec != HAL_VIDCODEC_JPG && 
        (ret = i6f_venc.fnStartReceiving(device, index)))
        return ret;

    i6f_state[index].payload = config->codec;

    return EXIT_SUCCESS;
}

int i6f_encoder_destroy(char index, char jpeg)
{
    int ret;
    char device = jpeg ? I6F_VENC_DEV_MJPG_0 : I6F_VENC_DEV_H26X_0;

    i6f_state[index].payload = HAL_VIDCODEC_UNSPEC;

    if (ret = i6f_venc.fnStopReceiving(device, index))
        return ret;

    {
        i6f_sys_bind source = { .module = I6F_SYS_MOD_SCL, 
            .device = _i6f_scl_dev, .channel = _i6f_scl_chn, .port = index };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_VENC,
            .device = device, .channel = index, .port = _i6f_venc_port };
        if (ret = i6f_sys.fnUnbind(0, &source, &dest))
            return ret;
    }

    if (ret = i6f_venc.fnDestroyChannel(device, index))
        return ret;
    
    if (ret = i6f_scl.fnDisablePort(_i6f_scl_dev, _i6f_scl_chn, index))
        return ret;
    
    return EXIT_SUCCESS;
}

int i6f_encoder_destroy_all(void)
{
    int ret;

    for (char i = 0; i < I6F_VENC_CHN_NUM; i++)
        if (i6f_state[i].enable)
            if (ret = i6f_encoder_destroy(i, 
                i6f_state[i].payload == HAL_VIDCODEC_JPG || 
                i6f_state[i].payload == HAL_VIDCODEC_MJPG))
                return ret;
    
    return EXIT_SUCCESS;
}

int i6f_encoder_snapshot_grab(char index, short width, short height,
    char quality, char grayscale, hal_jpegdata *jpeg)
{
    int ret;
    char device = 
        (i6f_state[index].payload == HAL_VIDCODEC_JPG ||
         i6f_state[index].payload == HAL_VIDCODEC_MJPG) ? 
         I6F_VENC_DEV_MJPG_0 : I6F_VENC_DEV_H26X_0;

    if (ret = i6f_channel_bind(index, 1, 1)) {
        fprintf(stderr, "[i6f_venc] Binding the encoder channel "
            "%d failed with %#x!\n", index, ret);
        goto abort;
    }
    return ret;

    i6f_venc_jpg param;
    memset(&param, 0, sizeof(param));
    if (ret = i6f_venc.fnGetJpegParam(device, index, &param)) {
        fprintf(stderr, "[i6f_venc] Reading the JPEG settings "
            "%d failed with %#x!\n", index, ret);
        goto abort;
    }
    return ret;
        return ret;
    param.quality = quality;
    if (ret = i6f_venc.fnSetJpegParam(device, index, &param)) {
        fprintf(stderr, "[i6f_venc] Writing the JPEG settings "
            "%d failed with %#x!\n", index, ret);
        goto abort;
    }

    i6f_channel_grayscale(grayscale);

    unsigned int count = 1;
    if (i6f_venc.fnStartReceivingEx(device, index, &count)) {
        fprintf(stderr, "[i6f_venc] Requesting one frame "
            "%d failed with %#x!\n", index, ret);
        goto abort;
    }

    int fd = i6f_venc.fnGetDescriptor(device, index);

    struct timeval timeout = { .tv_sec = 2, .tv_usec = 0 };
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(fd, &readFds);
    ret = select(fd + 1, &readFds, NULL, NULL, &timeout);
    if (ret < 0) {
        fprintf(stderr, "[i6f_venc] Select operation failed!\n");
        goto abort;
    } else if (ret == 0) {
        fprintf(stderr, "[i6f_venc] Capture stream timed out!\n");
        goto abort;
    }

    if (FD_ISSET(fd, &readFds)) {
        i6f_venc_stat stat;
        if (i6f_venc.fnQuery(device, index, &stat)) {
            fprintf(stderr, "[i6f_venc] Querying the encoder channel "
                "%d failed with %#x!\n", index, ret);
            goto abort;
        }

        if (!stat.curPacks) {
            fprintf(stderr, "[i6f_venc] Current frame is empty, skipping it!\n");
            goto abort;
        }

        i6f_venc_strm strm;
        memset(&strm, 0, sizeof(strm));
        strm.packet = (i6f_venc_pack*)malloc(sizeof(i6f_venc_pack) * stat.curPacks);
        if (!strm.packet) {
            fprintf(stderr, "[i6f_venc] Memory allocation on channel %d failed!\n", index);
            goto abort;
        }
        strm.count = stat.curPacks;

        if (ret = i6f_venc.fnGetStream(device, index, &strm, stat.curPacks)) {
            fprintf(stderr, "[i6f_venc] Getting the stream on "
                "channel %d failed with %#x!\n", index, ret);
            free(strm.packet);
            strm.packet = NULL;
            goto abort;
        }

        {
            jpeg->jpegSize = 0;
            for (unsigned int i = 0; i < strm.count; i++) {
                i6f_venc_pack *pack = &strm.packet[i];
                unsigned int packLen = pack->length - pack->offset;
                unsigned char *packData = pack->data + pack->offset;

                unsigned int newLen = jpeg->jpegSize + packLen;
                if (newLen > jpeg->length) {
                    jpeg->data = realloc(jpeg->data, newLen);
                    jpeg->length = newLen;
                }
                memcpy(jpeg->data + jpeg->jpegSize, packData, packLen);
                jpeg->jpegSize += packLen;
            }
        }

abort:
        i6f_venc.fnFreeStream(device, index, &strm);
    }

    i6f_venc.fnFreeDescriptor(device, index);

    i6f_venc.fnStopReceiving(device, index);

    i6f_channel_unbind(device, index);

    return ret;    
}

void *i6f_encoder_thread(void)
{
    int ret;
    int maxFd = 0;

    for (int i = 0; i < I6F_VENC_CHN_NUM; i++) {
        if (!i6f_state[i].enable) continue;
        if (!i6f_state[i].mainLoop) continue;
        char device = 
            (i6f_state[i].payload == HAL_VIDCODEC_JPG ||
             i6f_state[i].payload == HAL_VIDCODEC_MJPG) ? 
             I6F_VENC_DEV_MJPG_0 : I6F_VENC_DEV_H26X_0;

        ret = i6f_venc.fnGetDescriptor(device, i);
        if (ret < 0) {
            fprintf(stderr, "[i6f_venc] Getting the encoder descriptor failed with %#x!\n", ret);
            return NULL;
        }
        i6f_state[i].fileDesc = ret;

        if (maxFd <= i6f_state[i].fileDesc)
            maxFd = i6f_state[i].fileDesc;
    }

    i6f_venc_stat stat;
    i6f_venc_strm stream;
    struct timeval timeout;
    fd_set readFds;

    while (keepRunning) {
        FD_ZERO(&readFds);
        for(int i = 0; i < I6F_VENC_CHN_NUM; i++) {
            if (!i6f_state[i].enable) continue;
            if (!i6f_state[i].mainLoop) continue;
            FD_SET(i6f_state[i].fileDesc, &readFds);
        }

        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        ret = select(maxFd + 1, &readFds, NULL, NULL, &timeout);
        if (ret < 0) {
            fprintf(stderr, "[i6f_venc] Select operation failed!\n");
            break;
        } else if (ret == 0) {
            fprintf(stderr, "[i6f_venc] Main stream loop timed out!\n");
            continue;
        } else {
            for (int i = 0; i < I6F_VENC_CHN_NUM; i++) {
                if (!i6f_state[i].enable) continue;
                if (!i6f_state[i].mainLoop) continue;
                if (FD_ISSET(i6f_state[i].fileDesc, &readFds)) {
                    char device = 
                        (i6f_state[i].payload == HAL_VIDCODEC_JPG ||
                         i6f_state[i].payload == HAL_VIDCODEC_MJPG) ? 
                         I6F_VENC_DEV_MJPG_0 : I6F_VENC_DEV_H26X_0;

                    memset(&stream, 0, sizeof(stream));
                    
                    if (ret = i6f_venc.fnQuery(device, i, &stat)) {
                        fprintf(stderr, "[i6f_venc] Querying the encoder channel "
                            "%d failed with %#x!\n", i, ret);
                        break;
                    }

                    if (!stat.curPacks) {
                        fprintf(stderr, "[i6f_venc] Current frame is empty, skipping it!\n");
                        continue;
                    }

                    stream.packet = (i6f_venc_pack*)malloc(
                        sizeof(i6f_venc_pack) * stat.curPacks);
                    if (!stream.packet) {
                        fprintf(stderr, "[i6f_venc] Memory allocation on channel %d failed!\n", i);
                        break;
                    }
                    stream.count = stat.curPacks;

                    if (ret = i6f_venc.fnGetStream(device, i, &stream, stat.curPacks)) {
                        fprintf(stderr, "[i6f_venc] Getting the stream on "
                            "channel %d failed with %#x!\n", i, ret);
                        break;
                    }

                    if (i6f_venc_cb) {
                        hal_vidstream outStrm;
                        hal_vidpack outPack[stat.curPacks];
                        outStrm.count = stream.count;
                        outStrm.seq = stream.sequence;
                        for (int j = 0; j < stat.curPacks; j++) {
                            outPack[j].data = stream.packet[j].data;
                            outPack[j].length = stream.packet[j].length;
                            outPack[j].offset = stream.packet[j].offset;
                        }
                        outStrm.pack = outPack;
                        (*i6f_venc_cb)(i, &outStrm);
                    }

                    if (ret = i6f_venc.fnFreeStream(device, i, &stream)) {
                        fprintf(stderr, "[i6f_venc] Releasing the stream on "
                            "channel %d failed with %#x!\n", i, ret);
                    }
                    free(stream.packet);
                    stream.packet = NULL;
                }
            }
        }
    }
    fprintf(stderr, "[i6f_venc] Shutting down encoding thread...\n");
}

int i6f_pipeline_create(char sensor, short width, short height, char framerate)
{
    int ret;

    _i6f_snr_index = sensor;
    _i6f_snr_profile = -1;

    {
        unsigned int count;
        i6f_snr_res resolution;
        if (ret = i6f_snr.fnSetPlaneMode(_i6f_snr_index, 0))
            return ret;

        if (ret = i6f_snr.fnGetResolutionCount(_i6f_snr_index, &count))
            return ret;
        for (char i = 0; i < count; i++) {
            if (ret = i6f_snr.fnGetResolution(_i6f_snr_index, i, &resolution))
                return ret;

            if (width > resolution.crop.width ||
                height > resolution.crop.height ||
                framerate > resolution.maxFps)
                continue;
        
            _i6f_snr_profile = i;
            if (ret = i6f_snr.fnSetResolution(_i6f_snr_index, _i6f_snr_profile))
                return ret;
            _i6f_snr_framerate = framerate;
            if (ret = i6f_snr.fnSetFramerate(_i6f_snr_index, _i6f_snr_framerate))
                return ret;
            break;
        }
        if (_i6f_snr_profile < 0)
            return EXIT_FAILURE;
    }

    if (ret = i6f_snr.fnGetPadInfo(_i6f_snr_index, &_i6f_snr_pad))
        return ret;
    if (ret = i6f_snr.fnGetPlaneInfo(_i6f_snr_index, 0, &_i6f_snr_plane))
        return ret;
    if (ret = i6f_snr.fnEnable(_i6f_snr_index))
        return ret;

    {
        i6f_vif_grp group;
        group.intf = _i6f_snr_pad.intf;
        group.work = I6F_VIF_WORK_1MULTIPLEX;
        group.hdr = I6F_HDR_OFF;
        group.edge = group.intf == I6F_INTF_BT656 ?
            _i6f_snr_pad.intfAttr.bt656.edge : I6F_EDGE_DOUBLE;
        group.interlaceOn = 0;
        group.grpStitch = (1 << _i6f_vif_grp);
        if (ret = i6f_vif.fnCreateGroup(_i6f_vif_grp, &group))
            return ret;
    }
    
    {
        i6f_vif_dev device;
        device.pixFmt = (i6f_common_pixfmt)(_i6f_snr_plane.bayer > I6F_BAYER_END ? 
            _i6f_snr_plane.pixFmt : (I6F_PIXFMT_RGB_BAYER + _i6f_snr_plane.precision * I6F_BAYER_END + _i6f_snr_plane.bayer));
        device.crop = _i6f_snr_plane.capt;
        device.field = 0;
        device.halfHScan = 0;
        if (ret = i6f_vif.fnSetDeviceConfig(_i6f_vif_dev, &device))
            return ret;
    }
    if (ret = i6f_vif.fnEnableDevice(_i6f_vif_dev))
        return ret;

    {
        i6f_vif_port port;
        port.capt = _i6f_snr_plane.capt;
        port.dest.height = _i6f_snr_plane.capt.height;
        port.dest.width = _i6f_snr_plane.capt.width;
        port.pixFmt = (i6f_common_pixfmt)(_i6f_snr_plane.bayer > I6F_BAYER_END ? 
            _i6f_snr_plane.pixFmt : (I6F_PIXFMT_RGB_BAYER + _i6f_snr_plane.precision * I6F_BAYER_END + _i6f_snr_plane.bayer));
        port.frate = I6F_VIF_FRATE_FULL;
        if (ret = i6f_vif.fnSetPortConfig(_i6f_vif_dev, _i6f_vif_chn, &port))
            return ret;
    }
    if (ret = i6f_vif.fnEnablePort(_i6f_vif_dev, _i6f_vif_chn))
        return ret;

    {
        unsigned int combo = 1;
        if (ret = i6f_isp.fnCreateDevice(_i6f_isp_dev, &combo))
            return ret;
    }

    {
        i6f_isp_chn channel;
        memset(&channel, 0, sizeof(channel));
        channel.sensorId = (1 << _i6f_snr_index);
        if (ret = i6f_isp.fnCreateChannel(_i6f_isp_dev, _i6f_isp_chn, &channel))
            return ret;
    }

    {
        i6f_isp_para param;
        param.hdr = I6F_HDR_OFF;
        param.level3DNR = 0;
        param.mirror = 0;
        param.flip = 0;
        param.rotate = 0;
        if (ret = i6f_isp.fnSetChannelParam(_i6f_isp_dev, _i6f_isp_chn, &param))
            return ret;
    }
    if (ret = i6f_isp.fnStartChannel(_i6f_isp_dev, _i6f_isp_chn))
        return ret;

    {
        i6f_isp_port port;
        memset(&port, 0, sizeof(port));
        port.pixFmt = I6F_PIXFMT_YUV422_YUYV;
        if (ret = i6f_isp.fnSetPortConfig(_i6f_isp_dev, _i6f_isp_chn, _i6f_isp_port, &port))
            return ret;
    }
    if (ret = i6f_isp.fnEnablePort(_i6f_isp_dev, _i6f_isp_chn, _i6f_isp_port))
        return ret;

    {
        unsigned int binds = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
        if (ret = i6f_scl.fnCreateDevice(_i6f_scl_dev, &binds))
            return ret;
    }

    {
        unsigned int reserved = 0;
        if (ret = i6f_scl.fnCreateChannel(_i6f_scl_dev, _i6f_scl_chn, &reserved))
            return ret;
    }
    {
        int rotate = 0;
        if (ret = i6f_scl.fnAdjustChannelRotation(_i6f_scl_dev, _i6f_scl_chn, &rotate))
            return ret;
    }
    if (ret = i6f_scl.fnStartChannel(_i6f_scl_dev, _i6f_scl_chn))
        return ret;

    {
        i6f_sys_bind source = { .module = I6F_SYS_MOD_VIF, 
            .device = _i6f_vif_dev, .channel = _i6f_vif_chn, .port = 0 };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_ISP,
            .device = _i6f_isp_dev, .channel = _i6f_isp_chn, .port = _i6f_isp_port };
        if (ret = i6f_sys.fnBindExt(0, &source, &dest, _i6f_snr_framerate, _i6f_snr_framerate,
            I6F_SYS_LINK_REALTIME, 0))
            return ret;
    }

    {
        i6f_sys_bind source = { .module = I6F_SYS_MOD_ISP, 
            .device = _i6f_isp_dev, .channel = _i6f_isp_chn, .port = _i6f_isp_port };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_SCL,
            .device = _i6f_scl_dev, .channel = _i6f_scl_chn, .port = 0 };
        return i6f_sys.fnBindExt(0, &source, &dest, _i6f_snr_framerate, _i6f_snr_framerate,
            I6F_SYS_LINK_REALTIME, 0);
    }

    return EXIT_SUCCESS;
}

void i6f_pipeline_destroy(void)
{
    for (char i = 0; i < 4; i++)
        i6f_scl.fnDisablePort(_i6f_scl_dev, _i6f_scl_chn, i);

    {
        i6f_sys_bind source = { .module = I6F_SYS_MOD_ISP, 
            .device = _i6f_isp_dev, .channel = _i6f_isp_chn, .port = _i6f_isp_port };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_SCL,
            .device = _i6f_scl_dev, .channel = _i6f_scl_chn, .port = 0 };
        i6f_sys.fnUnbind(0, &source, &dest);
    }

    i6f_scl.fnStopChannel(_i6f_scl_dev, _i6f_scl_chn);
    i6f_scl.fnDestroyChannel(_i6f_scl_dev, _i6f_scl_chn);

    i6f_scl.fnDestroyDevice(_i6f_scl_dev);

    i6f_isp.fnStopChannel(_i6f_isp_dev, _i6f_isp_chn);
    i6f_isp.fnDestroyChannel(_i6f_isp_dev, _i6f_isp_chn);

    i6f_isp.fnDestroyDevice(_i6f_isp_dev);

    {   
        i6f_sys_bind source = { .module = I6F_SYS_MOD_VIF, 
            .device = _i6f_vif_dev, .channel = _i6f_vif_chn, .port = 0 };
        i6f_sys_bind dest = { .module = I6F_SYS_MOD_ISP,
            .device = _i6f_isp_dev, .channel = _i6f_isp_chn, .port = _i6f_isp_port };
        i6f_sys.fnUnbind(0, &source, &dest);
    }

    i6f_vif.fnDisablePort(_i6f_vif_dev, 0);

    i6f_vif.fnDisableDevice(_i6f_vif_dev);

    i6f_snr.fnDisable(_i6f_snr_index);
}

int i6f_region_create(char handle, hal_rect rect)
{
    int ret;

    i6f_sys_bind channel = { .module = I6F_SYS_MOD_SCL,
        .device = _i6f_scl_dev, .channel = _i6f_scl_chn };
    i6f_rgn_cnf region, regionCurr;
    i6f_rgn_chn attrib, attribCurr;

    region.type = I6F_RGN_TYPE_OSD;
    region.pixFmt = I6F_RGN_PIXFMT_ARGB1555;
    region.size.width = rect.width;
    region.size.height = rect.height;
    if (ret = i6f_rgn.fnGetRegionConfig(0, handle, &regionCurr)) {
        fprintf(stderr, "[i6f_rgn] Creating region %d...\n", handle);
        if (ret = i6f_rgn.fnCreateRegion(0, handle, &region))
            return ret;
    } else if (regionCurr.size.height != region.size.height || 
        regionCurr.size.width != region.size.width) {
        fprintf(stderr, "[i6f_rgn] Parameters are different, recreating "
            "region %d...\n", handle);
        channel.port = 1;
        i6f_rgn.fnDetachChannel(0, handle, &channel);
        channel.port = 0;
        i6f_rgn.fnDetachChannel(0, handle, &channel);
        i6f_rgn.fnDestroyRegion(0, handle);
        if (ret = i6f_rgn.fnCreateRegion(0, handle, &region))
            return ret;
    }

    if (i6f_rgn.fnGetChannelConfig(0, handle, &channel, &attribCurr))
        fprintf(stderr, "[i6f_rgn] Attaching region %d...\n", handle);
    else if (attribCurr.point.x != rect.x || attribCurr.point.x != rect.y) {
        fprintf(stderr, "[i6f_rgn] Position has changed, reattaching "
            "region %d...\n", handle);
        channel.port = 1;
        i6f_rgn.fnDetachChannel(0, handle, &channel);
        channel.port = 0;
        i6f_rgn.fnDetachChannel(0, handle, &channel);
    }

    memset(&attrib, 0, sizeof(attrib));
    attrib.show = 1;
    attrib.point.x = rect.x;
    attrib.point.y = rect.y;
    attrib.osd.layer = 0;
    attrib.osd.constAlphaOn = 0;
    attrib.osd.bgFgAlpha[0] = 0;
    attrib.osd.bgFgAlpha[1] = 255;

    channel.port = 0;
    i6f_rgn.fnAttachChannel(0, handle, &channel, &attrib);
    channel.port = 1;
    i6f_rgn.fnAttachChannel(0, handle, &channel, &attrib);

    return ret;
}

void i6f_region_deinit(void)
{
    i6f_rgn.fnDeinit(0);
}

void i6f_region_destroy(char handle)
{
    i6f_sys_bind channel = { .module = I6F_SYS_MOD_SCL,
        .device = _i6f_scl_dev, .channel = _i6f_scl_chn };
    
    channel.port = 1;
    i6f_rgn.fnDetachChannel(0, handle, &channel);
    channel.port = 0;
    i6f_rgn.fnDetachChannel(0, handle, &channel);
    i6f_rgn.fnDestroyRegion(0, handle);
}

void i6f_region_init(void)
{
    i6f_rgn_pal palette = {{{0, 0, 0, 0}}};
    i6f_rgn.fnInit(0, &palette);
}

int i6f_region_setbitmap(int handle, hal_bitmap *bitmap)
{
    i6f_rgn_bmp nativeBmp = { .data = bitmap->data, .pixFmt = I6F_RGN_PIXFMT_ARGB1555,
        .size.height = bitmap->dim.height, .size.width = bitmap->dim.width };

    return i6f_rgn.fnSetBitmap(0, handle, &nativeBmp);
}

void i6f_system_deinit(void)
{
    i6f_sys.fnExit(0);
}

int i6f_system_init(void)
{
    int ret;

    {
        i6f_sys_ver version;
        if (ret = i6f_sys.fnGetVersion(0, &version))
            return ret;
        printf("App built with headers v%s\n", I6F_SYS_API);
        puts(version.version);
    }

    if (ret = i6f_sys.fnInit(0))
        return ret;

    return EXIT_SUCCESS;
}