// https://code.videolan.org/gsoc/GSoC2017/shalzz/vlc/-/raw/master/test/fuzz/libvlc_decoder_fuzzer.cpp

/*****************************************************************************
 * libvlc_decoder_fuzzer.cpp: fuzz target for decoder modules
 *****************************************************************************
 * Copyright (C) 2017 VideoLAN
 *
 * Authors: Shaleen Jain <shaleen.jain95@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*
 * Fuzz decoder modules of various codecs. Specify which on the
 * command line:
 *
 * libvlc_decoder_fuzzer --codec=<four character codec>
 */

#include <vlc_common.h>
#include <vlc_modules.h>
#include <vlc_codec.h>
#include <vlc_block.h>
#include <assert.h>

#include <vlc/libvlc.h>
#include "../../lib/libvlc_internal.h"

extern "C" {
#include "fuzzer.h"
}

vlc_fourcc_t codec_t;
static libvlc_instance_t *p_libvlc;

int FuzzerInitialize(int *argc, char ***argv) {

    int i_vlc_argc = 1;
    const char *ppsz_vlc_argv[i_vlc_argc];
    ppsz_vlc_argv[0] = "--avcodec-hw=none";

    setenv("VLC_PLUGIN_PATH", "../../modules", 1);
    p_libvlc = libvlc_new(i_vlc_argc, ppsz_vlc_argv);
    assert(p_libvlc != NULL);

    if( codec ) {
        codec_t = vlc_fourcc_GetCodecFromString(VIDEO_ES, codec );
        msg_Dbg(p_libvlc->p_libvlc_int, "%s", codec);
        assert(codec_t);
    } else {
        printf("usage: libvlc_decoder_fuzzer --codec=<codec>\n");
        exit(0);
    }

    libvlc_log_unset(p_libvlc);
    return 0;
}

static picture_t *video_new_buffer_decoder( decoder_t *p_dec ) {
    return picture_NewFromFormat( &p_dec->fmt_out.video );
}

static subpicture_t *spu_new_buffer_decoder( decoder_t *p_dec,
                                const subpicture_updater_t * p_subpic ) {
    return subpicture_New (p_subpic);
}

static int video_update_format_decoder( decoder_t *p_dec ) { return 0; }
static int queue_video( decoder_t *p_dec, picture_t *p_pic ) { return 0; }
static int queue_audio( decoder_t *p_dec, block_t *p_block ) { return 0; }
static int queue_cc( decoder_t *p_dec, block_t *p_block,
                                bool p_cc_present[4], int x) { return 0; }
static int queue_sub( decoder_t *p_dec, subpicture_t *p_subpic ) { return 0; }

static void block_stub_Release (block_t *block)
{
    block->p_next = NULL;
    free (block);
    block = NULL;
}

int FuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    decoder_t *p_packetizer = (decoder_t*)
        vlc_object_create(p_libvlc->p_libvlc_int, sizeof( *p_packetizer ));
    assert( p_packetizer );

    decoder_t *p_decoder = (decoder_t*)
        vlc_object_create(p_libvlc->p_libvlc_int, sizeof( *p_decoder ));
    assert( p_decoder );

    p_decoder->pf_vout_format_update = video_update_format_decoder;
    p_decoder->pf_vout_buffer_new = video_new_buffer_decoder;
    p_decoder->pf_spu_buffer_new = spu_new_buffer_decoder;
    p_decoder->pf_queue_video = queue_video;
    p_decoder->pf_queue_audio = queue_audio;
    p_decoder->pf_queue_cc = queue_cc;
    p_decoder->pf_queue_sub = queue_sub;
    p_decoder->b_frame_drop_allowed = true;
    p_decoder->i_extra_picture_buffers = 0;
    p_packetizer->b_frame_drop_allowed = true;
    p_packetizer->i_extra_picture_buffers = 0;

    es_format_Init( &p_packetizer->fmt_in, VIDEO_ES, codec_t );
    es_format_Init( &p_decoder->fmt_in, VIDEO_ES, codec_t );
    es_format_Init( &p_packetizer->fmt_out, UNKNOWN_ES, 0 );
    es_format_Init( &p_decoder->fmt_out, UNKNOWN_ES, 0 );

    p_packetizer->p_module = module_need( p_packetizer, "packetizer", "$packetizer", false );
    p_decoder->p_module = module_need( p_decoder, "video decoder", "$codec", false );
    assert(p_packetizer->p_module);
    assert(p_decoder->p_module);

    block_t *p_block = block_heap_Alloc(const_cast<uint8_t *>(Data), Size);
    p_block->pf_release = block_stub_Release;

    block_t *p_packetized_block;
    while( ( p_packetized_block =
                p_packetizer->pf_packetize(p_packetizer, &p_block) ) )
    {

        if( !es_format_IsSimilar( &p_decoder->fmt_in, &p_packetizer->fmt_out ) )
        {
            msg_Dbg( p_decoder, "restarting module due to input format change");

            /* Drain the decoder module */
            p_decoder->pf_decode( p_decoder, NULL );

            /* Reload decoder */
            module_unneed( p_decoder, p_decoder->p_module );
            es_format_Clean( &p_decoder->fmt_in );
            es_format_Copy( &p_decoder->fmt_in, &p_packetizer->fmt_out );
            p_decoder->p_module = module_need( p_decoder, "video decoder",
                                                        "$codec", false );

        }

        if (p_packetizer->pf_get_cc) {
            bool pb_present[4];
            int i_reorder_depth;
            p_packetizer->pf_get_cc( p_packetizer, pb_present, &i_reorder_depth );
        }

        while(p_packetized_block) {

            block_t *p_next = p_packetized_block->p_next;
            p_packetized_block->p_next = NULL;

            int ret = p_decoder->pf_decode( p_decoder, p_packetized_block );

            if (ret == VLCDEC_ECRITICAL) {
                block_ChainRelease( p_next );
                break;
            }

            p_packetized_block = p_next;
        }

    }

    p_packetized_block = p_packetizer->pf_packetize(p_packetizer, NULL);
    free(p_packetized_block);
    p_decoder->pf_decode( p_decoder, NULL );

    es_format_Clean (&p_packetizer->fmt_in);
    es_format_Clean (&p_packetizer->fmt_out);
    es_format_Clean (&p_decoder->fmt_in);
    es_format_Clean (&p_decoder->fmt_out);

    module_unneed( p_decoder, p_decoder->p_module );
    module_unneed( p_packetizer, p_packetizer->p_module );

    vlc_object_release( p_packetizer );
    vlc_object_release( p_decoder );
    return 0;
}

void FuzzerCleanup(void) {
    libvlc_release(p_libvlc);
}
