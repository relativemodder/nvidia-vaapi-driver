#include "vabackend.h"
#include <string.h>

// NVENC error checking function
bool checkNvencErrors(NVENCSTATUS err, const char *file, const char *function, const int line) {
    if (NV_ENC_SUCCESS != err) {
        logger(file, function, line, "NVENC ERROR '%d' (%d)\n", err, err);
        return true;
    }
    return false;
}

// H.264 encoding parameter handlers
static void copyH264EncPicParam(NVContext *ctx, NVBuffer* buffer, NV_ENC_PIC_PARAMS *picParams)
{
    VAPictureParameterBufferH264* buf = (VAPictureParameterBufferH264*) buffer->ptr;
    
    // Map VA-API H.264 parameters to NVENC parameters
    // This is a simplified mapping - real implementation would need more complex logic
    
    // Set basic encoding parameters
    picParams->encodePicFlags = 0;
    picParams->inputTimeStamp = 0;
    picParams->inputDuration = 0;
    picParams->pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    
    // Set H.264 specific parameters
    picParams->codecPicParams.h264PicParams.refPicFlag = (buf->pic_fields.bits.reference_pic_flag != 0);
    picParams->codecPicParams.h264PicParams.forceIntraRefreshWithFrameCnt = 0;
    
    // Set output bitstream
    picParams->outputBitstream = NULL; // Will be set by the encoding context
    picParams->completionEvent = NULL;
}

static void copyH264EncSliceParam(NVContext *ctx, NVBuffer* buffer, NV_ENC_PIC_PARAMS *picParams)
{
    // Store slice parameters for later use
    ctx->lastSliceParams = buffer->ptr;
    ctx->lastSliceParamsCount = buffer->elements;
    
    // This is a placeholder - real implementation would process slice parameters
    (void)picParams;
}

static void copyH264EncSliceData(NVContext *ctx, NVBuffer* buf, NV_ENC_PIC_PARAMS *picParams)
{
    // Copy slice data to the encoding buffer
    for (unsigned int i = 0; i < ctx->lastSliceParamsCount; i++) {
        VASliceParameterBufferH264 *sliceParams = &((VASliceParameterBufferH264*) ctx->lastSliceParams)[i];
        
        // Append slice data to the bitstream buffer
        appendBuffer(&ctx->bitstreamBuffer, 
                    PTROFF(buf->ptr, sliceParams->slice_data_offset), 
                    sliceParams->slice_data_size);
    }
    
    // This is a placeholder - real implementation would set slice data
    (void)picParams;
}

static void ignoreH264EncBuffer(NVContext *ctx, NVBuffer *buffer, NV_ENC_PIC_PARAMS *picParams)
{
    // Intentionally do nothing for unsupported buffer types
    (void)ctx;
    (void)buffer;
    (void)picParams;
}

// Compute NVENC GUID for H.264
static GUID computeH264NvencGUID(VAProfile profile) {
    if (profile == VAProfileH264ConstrainedBaseline || 
        profile == VAProfileH264Main || 
        profile == VAProfileH264High) {
        return NV_ENC_CODEC_H264_GUID;
    }
    
    // Return a default GUID for unsupported profiles
    GUID emptyGUID = {0};
    return emptyGUID;
}

// Compute CUDA codec for H.264 (for decoding compatibility)
static cudaVideoCodec computeH264CudaCodec(VAProfile profile) {
    if (profile == VAProfileH264ConstrainedBaseline || 
        profile == VAProfileH264Main || 
        profile == VAProfileH264High) {
        return cudaVideoCodec_H264;
    }
    return cudaVideoCodec_NONE;
}

// Supported H.264 profiles for encoding
static const VAProfile h264EncSupportedProfiles[] = {
    VAProfileH264ConstrainedBaseline,
    VAProfileH264Main,
    VAProfileH264High,
};

// H.264 encoding codec definition
const DECLARE_ENCODE_CODEC(h264EncCodec) = {
    .computeCudaCodec = computeH264CudaCodec,
    .computeNvencGUID = computeH264NvencGUID,
    .handlers = {
        [VAPictureParameterBufferType] = NULL, // Not used for encoding
        [VAIQMatrixBufferType] = NULL,
        [VASliceParameterBufferType] = NULL,
        [VASliceDataBufferType] = NULL,
    },
    .encodeHandlers = {
        [VAPictureParameterBufferType] = copyH264EncPicParam,
        [VAIQMatrixBufferType] = ignoreH264EncBuffer,
        [VASliceParameterBufferType] = copyH264EncSliceParam,
        [VASliceDataBufferType] = copyH264EncSliceData,
    },
    .supportedProfileCount = 0, // Not used for encoding
    .supportedProfiles = NULL,
    .supportedEncodeProfileCount = ARRAY_SIZE(h264EncSupportedProfiles),
    .supportedEncodeProfiles = h264EncSupportedProfiles,
};
