// https://code.videolan.org/gsoc/GSoC2017/shalzz/vlc/-/raw/master/test/fuzz/fuzzer_common.c
/*****************************************************************************
 * fuzzer_common.c: fuzzer common
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

#include <vlc/libvlc.h>
#include <getopt.h>
#include <string.h>

#include "fuzzer.h"

int disable_avformat = 0;
char* codec;
static struct option long_options[] = {
    {"disable-avformat", no_argument,       &disable_avformat, 1},
    {"codec",            required_argument, 0,                 2},
    {0,                  0,                 0,                 0}
};

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    int c;
    do {
        c = getopt_long(*argc, *argv, "", long_options, NULL);

        switch (c) {
            case 2:
                codec = strdup(optarg);
                break;
        }
    } while ( c != -1 );

    return FuzzerInitialize (NULL, 0);
}

int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len) {
    return FuzzerTestOneInput(buf, len);
}
