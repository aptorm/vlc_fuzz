// https://code.videolan.org/gsoc/GSoC2017/shalzz/vlc/-/raw/master/test/fuzz/fuzzer.h
/*****************************************************************************
 * fuzzer.h: fuzzer common
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

#ifndef FUZZER_H
#define FUZZER_H

extern int disable_avformat;
extern char* codec;

int FuzzerTestOneInput(const uint8_t *buf, size_t len);
int FuzzerInitialize(int *argc, char ***argv);
void FuzzerCleanup(void);

#endif /* FUZZER_H */
