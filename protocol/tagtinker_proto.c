/*
 * ESLPwn - ESL protocol helpers (implementation)
 *
 * Ported from furrtek/ESLPwn tools_python/pr.py and img2dm.py
 *
 * SPDX-License-Identifier: MIT
 */

#include "tagtinker_proto.h"
#include "../tagtinker_app.h"
#include <string.h>
#include <stdlib.h>

/* ── Helpers ────────────────────────────────────────────────── */

static void append_word(uint8_t* buf, size_t* pos, uint16_t value) {
    buf[(*pos)++] = (value >> 8) & 0xFF;
    buf[(*pos)++] = value & 0xFF;
}

static size_t terminate(uint8_t* buf, size_t len) {
    uint16_t crc = eslpwn_crc16(buf, len);
    buf[len]     = crc & 0xFF;
    buf[len + 1] = (crc >> 8) & 0xFF;
    return len + 2;
}

static size_t raw_frame(uint8_t* buf, uint8_t proto,
                        const uint8_t plid[4], uint8_t cmd) {
    buf[0] = proto;
    buf[1] = plid[3]; buf[2] = plid[2];
    buf[3] = plid[1]; buf[4] = plid[0];
    buf[5] = cmd;
    return 6;
}

static size_t mcu_frame(uint8_t* buf, const uint8_t plid[4], uint8_t cmd) {
    size_t p = raw_frame(buf, ESLPWN_PROTO_DM, plid, 0x34);
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = cmd;
    return p;
}

/* ── CRC-16 (poly 0x8408, init 0x8408) ─────────────────────── */

uint16_t eslpwn_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0x8408;
    for(size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(int b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
    return crc;
}

/* ── Barcode → PLID ─────────────────────────────────────────── */

bool eslpwn_barcode_to_plid(const char* barcode, uint8_t plid[4]) {
    if(!barcode || strlen(barcode) != 17) return false;
    for(int i = 2; i < 12; i++)
        if(barcode[i] < '0' || barcode[i] > '9') return false;

    uint32_t lo = 0, hi = 0;
    for(int i = 2; i < 7; i++)  lo = lo * 10 + (barcode[i] - '0');
    for(int i = 7; i < 12; i++) hi = hi * 10 + (barcode[i] - '0');

    uint32_t id = lo + (hi << 16);
    plid[0] = (id >> 8)  & 0xFF;
    plid[1] = id & 0xFF;
    plid[2] = (id >> 24) & 0xFF;
    plid[3] = (id >> 16) & 0xFF;
    return true;
}

/* ── Frame builders ─────────────────────────────────────────── */

size_t eslpwn_build_broadcast_page_frame(
    uint8_t* buf, uint8_t page, bool forever, uint16_t duration) {

    const uint8_t plid[4] = {0};
    size_t p = raw_frame(buf, ESLPWN_PROTO_DM, plid, 0x06);
    buf[p++] = ((page & 7) << 3) | 0x01 | (forever ? 0x80 : 0x00);
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = forever ? 0x00 : ((duration >> 8) & 0xFF);
    buf[p++] = forever ? 0x00 : (duration & 0xFF);
    return terminate(buf, p);
}

size_t eslpwn_build_broadcast_debug_frame(uint8_t* buf) {
    const uint8_t plid[4] = {0};
    size_t p = raw_frame(buf, ESLPWN_PROTO_DM, plid, 0x06);
    buf[p++] = 0xF1;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x0A;
    return terminate(buf, p);
}

size_t eslpwn_make_addressed_frame(
    uint8_t* buf, const uint8_t plid[4],
    const uint8_t* payload, size_t payload_len) {

    size_t p = raw_frame(buf, ESLPWN_PROTO_DM, plid, payload[0]);
    memcpy(&buf[p], payload + 1, payload_len - 1);
    p += payload_len - 1;
    return terminate(buf, p);
}

size_t eslpwn_make_ping_frame(uint8_t* buf, const uint8_t plid[4]) {
    size_t p = raw_frame(buf, ESLPWN_PROTO_DM, plid, 0x17);
    buf[p++] = 0x01;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    for(int i = 0; i < 22; i++) buf[p++] = 0x01;
    return terminate(buf, p);
}

size_t eslpwn_make_refresh_frame(uint8_t* buf, const uint8_t plid[4]) {
    size_t p = mcu_frame(buf, plid, 0x01);
    for(int i = 0; i < 22; i++) buf[p++] = 0x00;
    return terminate(buf, p);
}

size_t eslpwn_make_mcu_frame(
    uint8_t* buf, const uint8_t plid[4], uint8_t cmd) {
    return mcu_frame(buf, plid, cmd);
}

static void record_run(uint8_t* out, size_t* pos, size_t cap, uint32_t run_count) {
    uint8_t bits[32];
    int n = 0;
    uint32_t v = run_count;
    while(v) { bits[n++] = v & 1; v >>= 1; }
    for(int i = 0; i < n / 2; i++) {
        uint8_t t = bits[i]; bits[i] = bits[n - 1 - i]; bits[n - 1 - i] = t;
    }
    /* Prefix zeros (n-1 of them, skip leading 1) */
    for(int i = 1; i < n; i++)
        if(*pos < cap) out[(*pos)++] = 0;
    /* The bits themselves */
    for(int i = 0; i < n; i++)
        if(*pos < cap) out[(*pos)++] = bits[i];
}

size_t eslpwn_rle_compress(
    const uint8_t* pixels, size_t count,
    uint8_t* out, size_t out_cap, uint8_t* comp_type) {

    if(count == 0) { *comp_type = 0; return 0; }

    size_t pos = 0;
    if(pos < out_cap) out[pos++] = pixels[0];

    uint8_t run_pixel = pixels[0];
    uint32_t run_count = 1;

    for(size_t i = 1; i < count; i++) {
        if(pixels[i] == run_pixel) {
            run_count++;
        } else {
            record_run(out, &pos, out_cap, run_count);
            run_pixel = pixels[i];
            run_count = 1;
        }
    }
    if(run_count > 1) record_run(out, &pos, out_cap, run_count);

    if(pos < count) {
        *comp_type = 2;  /* RLE */
        return pos;
    }
    /* Compression didn't help — use raw */
    memcpy(out, pixels, count < out_cap ? count : out_cap);
    *comp_type = 0;
    return count < out_cap ? count : out_cap;
}

/* ── Image upload sequence builder ──────────────────────────── */

#define DATA_BYTES_PER_FRAME 20
#define DATA_BITS_PER_FRAME  (DATA_BYTES_PER_FRAME * 8)

void eslpwn_build_image_sequence(
    EslPwnApp* app,
    const uint8_t plid[4],
    const uint8_t* pixels,
    uint16_t width, uint16_t height,
    uint8_t page,
    uint16_t pos_x, uint16_t pos_y,
    uint16_t wake_repeats) {

    size_t pixel_count = (size_t)width * height;

    /* Prepare pixel data — optionally add color-clear plane */
    size_t total_pixels;
    uint8_t* combined = NULL;
    const uint8_t* compress_src;

    if(app->color_clear) {
        /* Two planes: BW + color-clear (removes red artifacts) */
        total_pixels = pixel_count * 2;
        combined = malloc(total_pixels);
        if(!combined) return;
        memcpy(combined, pixels, pixel_count);
        memset(combined + pixel_count, 1, pixel_count); /* 1 = Red Pigment OFF */
        compress_src = combined;
    } else {
        /* Single BW plane — faster, less data */
        total_pixels = pixel_count;
        compress_src = pixels;
    }

    /* Compress */
    size_t comp_cap = total_pixels + 256;
    uint8_t* comp_bits = malloc(comp_cap);
    if(!comp_bits) { if(combined) free(combined); return; }

    uint8_t comp_type;
    size_t comp_len = eslpwn_rle_compress(
        compress_src, total_pixels, comp_bits, comp_cap, &comp_type);

    const uint8_t* src_bits = (comp_type == 2) ? comp_bits : compress_src;
    size_t src_len = (comp_type == 2) ? comp_len : total_pixels;

    /* Pack bits into bytes */
    size_t padding = (DATA_BITS_PER_FRAME - (src_len % DATA_BITS_PER_FRAME)) % DATA_BITS_PER_FRAME;
    size_t padded_bits = src_len + padding;
    size_t padded_bytes = padded_bits / 8;

    uint8_t* data_bytes = calloc(padded_bytes, 1);
    if(!data_bytes) { free(comp_bits); free(combined); return; }

    for(size_t i = 0; i < src_len; i++) {
        size_t byte_idx = i / 8;
        size_t bit_idx  = 7 - (i % 8);
        if(byte_idx < padded_bytes && src_bits[i])
            data_bytes[byte_idx] |= (1 << bit_idx);
    }

    size_t frame_count = padded_bytes / DATA_BYTES_PER_FRAME;

    FURI_LOG_I("ESLPwn", "IMG %ux%u pg=%u comp=%u %zu->%zu frames=%zu",
        width, height, page, comp_type, total_pixels, padded_bytes, frame_count);

    /* Free temp buffers */
    free(comp_bits);
    if(combined) free(combined);

    /* Total: ping + params + N data + refresh */
    size_t total = 2 + frame_count + 1;

    app->frame_seq_count = total;
    app->frame_sequence = malloc(sizeof(uint8_t*) * total);
    app->frame_lengths  = malloc(sizeof(size_t) * total);
    app->frame_repeats  = malloc(sizeof(uint16_t) * total);

    if(!app->frame_sequence || !app->frame_lengths || !app->frame_repeats) {
        free(data_bytes);
        app->frame_seq_count = 0;
        return;
    }

    size_t idx = 0;

    /* 1. Ping */
    app->frame_sequence[idx] = malloc(ESLPWN_MAX_FRAME_SIZE);
    app->frame_lengths[idx]  = eslpwn_make_ping_frame(app->frame_sequence[idx], plid);
    app->frame_repeats[idx]  = wake_repeats;
    idx++;

    /* 2. Parameters (cmd 0x05) */
    app->frame_sequence[idx] = malloc(ESLPWN_MAX_FRAME_SIZE);
    size_t p = mcu_frame(app->frame_sequence[idx], plid, 0x05);
    append_word(app->frame_sequence[idx], &p, (uint16_t)padded_bytes);
    app->frame_sequence[idx][p++] = 0x00;
    app->frame_sequence[idx][p++] = comp_type;
    app->frame_sequence[idx][p++] = page;
    append_word(app->frame_sequence[idx], &p, width);
    append_word(app->frame_sequence[idx], &p, height);
    append_word(app->frame_sequence[idx], &p, pos_x);
    append_word(app->frame_sequence[idx], &p, pos_y);
    append_word(app->frame_sequence[idx], &p, 0x0000);
    app->frame_sequence[idx][p++] = 0x88;
    append_word(app->frame_sequence[idx], &p, 0x0000);
    for(int i = 0; i < 4; i++) app->frame_sequence[idx][p++] = 0x00;
    app->frame_lengths[idx] = terminate(app->frame_sequence[idx], p);
    app->frame_repeats[idx] = 1;
    idx++;

    /* 3..N+2. Data frames (cmd 0x20) */
    for(size_t fi = 0; fi < frame_count; fi++) {
        app->frame_sequence[idx] = malloc(ESLPWN_MAX_FRAME_SIZE);
        p = mcu_frame(app->frame_sequence[idx], plid, 0x20);
        append_word(app->frame_sequence[idx], &p, (uint16_t)fi);
        size_t start = fi * DATA_BYTES_PER_FRAME;
        memcpy(&app->frame_sequence[idx][p], &data_bytes[start], DATA_BYTES_PER_FRAME);
        p += DATA_BYTES_PER_FRAME;
        app->frame_lengths[idx] = terminate(app->frame_sequence[idx], p);
        app->frame_repeats[idx] = 3;  /* 3 repeats per data frame for reliability */
        idx++;
    }

    /* N+3. Refresh */
    app->frame_sequence[idx] = malloc(ESLPWN_MAX_FRAME_SIZE);
    app->frame_lengths[idx]  = eslpwn_make_refresh_frame(app->frame_sequence[idx], plid);
    app->frame_repeats[idx]  = 1;

    /* Copy first data frame for display in TX scene */
    if(app->frame_seq_count > 1) {
        memcpy(app->frame_buf, app->frame_sequence[1],
               app->frame_lengths[1] < ESLPWN_MAX_FRAME_SIZE
                   ? app->frame_lengths[1] : ESLPWN_MAX_FRAME_SIZE);
        app->frame_len = app->frame_lengths[1];
    }

    free(data_bytes);
}
