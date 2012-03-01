#include <libavcodec/vda.h>
#include <VideoDecodeAcceleration/VDADecoder.h>
#include "libmpcodecs/mp_image.h"

#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>

typedef struct {
    struct vda_context hw_ctx;
    vda_frame *top_frame;
    unsigned int img_fmt;
} mp_vda_t;

mp_vda_t *mpvda;

/*****************************************************************************
 * vda_CopyPicture: copy y420 CVPixelBuffer to picture_t
 *****************************************************************************/
static void vda_CopyYv12CVBuffer(mp_image_t *mpi, CVPixelBufferRef buffer,
                                 unsigned i_width,unsigned i_height)
{
    uint8_t *pp_plane[3];
    size_t  pi_pitch[3];

    CVPixelBufferLockBaseAddress(buffer, 0);

    for (int i = 0; i < 3; i++) {
        pp_plane[i] = CVPixelBufferGetBaseAddressOfPlane(buffer, i);
        pi_pitch[i] = CVPixelBufferGetBytesPerRowOfPlane(buffer, i);
    }

    // performcopy here

    CVPixelBufferUnlockBaseAddress( buffer, 0 );
}

/*****************************************************************************
 * vda_CopyPicture: copy 2vuy CVPixelBuffer to picture_t
 *****************************************************************************/
static void vda_CopyUyvyCVBuffer(mp_image_t *mpi, CVPixelBufferRef buffer)
{
    int i_plane, i_line, i_dst_stride, i_src_stride;
    uint8_t *p_dst, *p_src;

    CVPixelBufferLockBaseAddress( buffer, 0 );

    //for (i_plane = 0; i_plane < p_pic->i_planes; i_plane++) {
    //    p_dst = p_pic->p[i_plane].p_pixels;
    //    p_src = CVPixelBufferGetBaseAddressOfPlane(buffer, i_plane);
    //    i_dst_stride  = p_pic->p[i_plane].i_pitch;
    //    i_src_stride  = CVPixelBufferGetBytesPerRowOfPlane(buffer, i_plane);

    //    for (i_line = 0; i_line < p_pic->p[i_plane].i_visible_lines ; i_line+ ) {
    //        //vlc_memcpy( p_dst, p_src, i_src_stride );

    //        p_src += i_src_stride;
    //        p_dst += i_dst_stride;
    //    }
    //}

    CVPixelBufferUnlockBaseAddress( buffer, 0 );
}

static int vda_setup(void **pp_hw_ctx, int img_fmt,
    int i_width, int i_height )
{

    if ( mpvda->hw_ctx.width == i_width
         && mpvda->hw_ctx.height == i_height
         && mpvda->hw_ctx.decoder ) {
        *pp_hw_ctx = &mpvda->hw_ctx;
        //*img_fmt = &mpvda->img_fmt;
        return 0;
    }

    if (mpvda->hw_ctx.decoder) {
        ff_vda_destroy_decoder(&mpvda->hw_ctx);
        goto ok;
    }

    memset(&mpvda->hw_ctx, 0, sizeof(mpvda->hw_ctx));
    mpvda->hw_ctx.queue = NULL;
    mpvda->hw_ctx.width = i_width;
    mpvda->hw_ctx.height = i_height;
    mpvda->hw_ctx.format = 'avc1';
    mpvda->hw_ctx.cv_pix_fmt_type = kCVPixelFormatType_422YpCbCr8;

    if ( mpvda->hw_ctx.cv_pix_fmt_type == kCVPixelFormatType_420YpCbCr8Planar ) {
        mpvda->img_fmt = IMGFMT_YV12;
        //CopyInitCache(&mpvda->image_cache, i_width);
    } else {
        mpvda->img_fmt = IMGFMT_UYVY;
    }

ok:
    /* Setup the ffmpeg hardware context */
    *pp_hw_ctx = &mpvda->hw_ctx;
    //*pi_chroma = p_va->i_chroma;

    /* create the decoder */
    int status = ff_vda_create_decoder(&p_va->hw_ctx, p_va->p_extradata, p_va->i_extradata);
    if (status) {
        msg_Err( p_va->p_log, "Failed to create the decoder : %i", status );
        return -1;
    }

    return 0;
}

static int vda_get(AVFrame *p_ff) {
    /* FIXME: release previous frame buffer if needed. The same part of code in Release causes memory leaks. */
    if (mpvda->top_frame)
        ff_vda_release_vda_frame(mpvda->top_frame);

    mpvda->top_frame = ff_vda_queue_pop(&mpvda->hw_ctx);

    /* */
    for(int i = 0; i < 4; i++){
        p_ff->data[i] = NULL;
        p_ff->linesize[i] = 0;

        if( i == 0 || i == 3 )
          p_ff->data[i] = 1; // FIXME: a better fake value ?
    }

    return VLC_SUCCESS;
}

static int vda_extract_frame(mp_image_t *mpi)
{
    if ( !mpvda->top_frame ) {
        //msg_Dbg( p_va->p_log, "Decoder is buffering...");
        return -1;
    }

    CVPixelBufferRef cv_buffer = mpvda->top_frame->cv_buffer;

    if (mpvda->hw_ctx.cv_pix_fmt_type == kCVPixelFormatType_420YpCbCr8Planar) {
        if (!mpvda->image_cache.buffer)
            return -1;

        vda_CopyYv12CVBuffer(mpi, cv_buffer,
                             mpvda->hw_ctx.width,
                             mpvda->hw_ctx.height,
                             &mpvda->image_cache);
    } else {
        vda_CopyUyvyCVBuffer( p_picture, cv_buffer );
    }

    //p_picture->date = p_va->top_frame->pts;

    return 0;
}

static void vda_close()
{
    ff_vda_destroy_decoder(&mpvda->hw_ctx) ;

    if (mpvda->top_frame)
        ff_vda_release_vda_frame(mpvda->top_frame);

    if (mpvda->hw_ctx.cv_pix_fmt_type == kCVPixelFormatType_420YpCbCr8Planar)
        CopyCleanCache(&mpvda->image_cache);

    freep(mpvda);
}
