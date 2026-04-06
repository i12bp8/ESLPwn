/*
 * TagTinker - ESL protocol helpers
 *
 * Frame construction, CRC, and encoding for the infrared
 * ESL protocol used by this project. Ported from furrtek/TagTinker.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TAGTINKER_PROTO_DM  0x85   /* Dot-matrix / graphic ESLs */
#define TAGTINKER_PROTO_SEG 0x84   /* 7-segment ESLs             */
#define TAGTINKER_MAX_FRAME_SIZE 96

/* Forward declaration for TagTinkerApp (avoids circular include) */
typedef struct TagTinkerApp TagTinkerApp;

/* ── CRC ────────────────────────────────────────────────────── */

uint16_t tagtinker_crc16(const uint8_t* data, size_t len);

/* ── Barcode / PLID ─────────────────────────────────────────── */

bool tagtinker_barcode_to_plid(const char* barcode, uint8_t plid[4]);

/* ── Frame builders ─────────────────────────────────────────── */

/* Broadcast page-change (no barcode needed). */
size_t tagtinker_build_broadcast_page_frame(
    uint8_t* buf, uint8_t page, bool forever, uint16_t duration);

/* Broadcast diagnostic page. */
size_t tagtinker_build_broadcast_debug_frame(uint8_t* buf);

/* Addressed DM frame: wraps raw payload with protocol + PLID + CRC. */
size_t tagtinker_make_addressed_frame(
    uint8_t* buf, const uint8_t plid[4],
    const uint8_t* payload, size_t payload_len);

/* Wake-up ping (must be sent before most addressed commands). */
size_t tagtinker_make_ping_frame(uint8_t* buf, const uint8_t plid[4]);

/* Display refresh request. */
size_t tagtinker_make_refresh_frame(uint8_t* buf, const uint8_t plid[4]);

/* MCU-level frame (used for image upload protocol). */
size_t tagtinker_make_mcu_frame(
    uint8_t* buf, const uint8_t plid[4], uint8_t cmd);

/* ── Image upload helpers ───────────────────────────────────── */

/* RLE-compress a pixel array (0/1 values).
 * Returns compressed bitstream length.  comp_type is set to 0 (raw) or 2 (RLE). */
size_t tagtinker_rle_compress(
    const uint8_t* pixels, size_t count,
    uint8_t* out, size_t out_cap, uint8_t* comp_type);

/* Build a complete image-upload frame sequence and store it in app state.
 * Allocates memory that the transmit scene frees on exit. */
void tagtinker_build_image_sequence(
    TagTinkerApp* app,
    const uint8_t plid[4],
    const uint8_t* pixels,
    uint16_t width, uint16_t height,
    uint8_t page,
    uint16_t pos_x, uint16_t pos_y,
    uint16_t wake_repeats);
