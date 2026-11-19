#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include <x264.h>

extern "C" {

__attribute__((visibility("default")))
x264_t *x264_encoder_open_165(x264_param_t *param) {
    static x264_t *(*real_fn)(x264_param_t *) = nullptr;
    if (!real_fn) {
        void *h = dlopen("libx264.so.165", RTLD_LAZY);
        if (h) real_fn = reinterpret_cast<x264_t *(*)(x264_param_t *)>(dlsym(h, "x264_encoder_open_165"));
        if (!real_fn) real_fn = reinterpret_cast<x264_t *(*)(x264_param_t *)>(dlsym(RTLD_NEXT, "x264_encoder_open_165"));
    }

    if (param) {
        // High-speed real-time webcam recording (>150-200 fps throughput):
        param->analyse.i_me_method = X264_ME_DIA; // Diamond motion estimation
        param->analyse.i_subpel_refine = 1;      // Fast subpixel refinement
        param->i_frame_reference = 1;            // 1 reference frame
        param->analyse.inter = 0;                // Fast inter partitions (P16x16)
        param->analyse.intra = 0;                // Fast intra partitions (I16x16)
        param->analyse.b_mixed_references = 0;
        param->analyse.i_trellis = 0;
        param->analyse.b_fast_pskip = 1;
        param->analyse.b_weighted_bipred = 0;

        // Low-latency pipeline:
        // Disable B-frames and lookahead to prevent encoder queue delay.
        param->i_bframe = 0;
        param->b_sliced_threads = 0;             // Frame threading for maximum multi-core throughput
        param->rc.i_lookahead = 0;
        param->rc.b_mb_tree = 0;

        // Fix Qt FFmpeg timebase mismatch:
        // Qt passes AVFrame PTS with timebase 1/15360 (512 per frame at 30 fps),
        // but sets avctx->time_base to 1/30. This caused x264 to think each frame
        // was 17 seconds long, calculating 189 kbps instead of 100 Mbps, which
        // broke VBV rate limiting and caused 280 MB file sizes.
        // Fix timebase to match the PTS scale:
        if (param->i_fps_num > 0) {
            param->i_timebase_num = 1;
            param->i_timebase_den = param->i_fps_num * 512;
        }

        // Rate control & file size optimization:
        // Default Qt sets CRF 19 without any VBV ceiling. On noisy webcam sensors,
        // unconstrained CRF 19 causes extreme bitrate spikes (~100 Mbps, ~280 MB for 20s).
        // Enforce CRF 23 for high visual quality with reasonable baseline size.
        if (param->rc.i_rc_method == X264_RC_CRF) {
            if (param->rc.f_rf_constant < 23.0f) {
                param->rc.f_rf_constant = 23.0f;
            }
        }

        // Cap maximum bitrate with VBV (video buffer verifier) to prevent runaway sizes:
        // <= 720p/736p: max 4.5 Mbps (~11 MB for 21 seconds instead of 280 MB)
        // > 720p (1080p): max 7 Mbps (~17 MB for 21 seconds)
        int h = (param->i_height > 0) ? param->i_height : 720;
        int maxBitrateKbps = (h <= 736) ? 4500 : 7000;
        if (param->rc.i_vbv_max_bitrate == 0 || param->rc.i_vbv_max_bitrate > maxBitrateKbps) {
            param->rc.i_vbv_max_bitrate = maxBitrateKbps;
            param->rc.i_vbv_buffer_size = maxBitrateKbps * 2;
        }
    }

    return real_fn ? real_fn(param) : nullptr;
}

}
