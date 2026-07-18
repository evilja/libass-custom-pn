/*
 * Copyright (C) 2026 libass contributors
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ass.h"
#include "ass_metrics.h"

#ifndef TEST_FONT_PATH
#error TEST_FONT_PATH must name a bundled test font
#endif

static char script[] =
    "[Script Info]\n"
    "ScriptType: v4.00+\n"
    "PlayResX: 640\n"
    "PlayResY: 360\n"
    "\n"
    "[V4+ Styles]\n"
    "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, "
        "OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, "
        "ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, "
        "Alignment, MarginL, MarginR, MarginV, Encoding\n"
    "Style: Default,sans-serif,40,&H00FFFFFF,&H000000FF,&H00000000,"
        "&H00000000,0,0,0,0,100,100,0,0,1,0,0,2,10,10,10,1\n"
    "\n"
    "[Events]\n"
    "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, "
        "Effect, Text\n"
    "Dialogue: 0,0:00:00.00,0:00:01.00,Default,,0,0,0,,"
        "{\\p1}m 0 0 l 10 0 10 10 0 10\n"
    "Dialogue: 0,0:00:02.00,0:00:03.00,Default,,0,0,0,,{comment}\n"
    "Dialogue: 0,0:00:04.00,0:00:05.00,Default,,0,0,0,,"
        "{\\p1}m 0 0 l 10 0 10 10 0 10\n"
    "Dialogue: 0,0:00:04.00,0:00:05.00,Default,,0,0,0,,"
        "{\\p1}m 0 0 l 20 0 20 20 0 20\n"
    "Dialogue: 0,0:00:06.00,0:00:07.00,Default,,0,0,0,,q\n"
    "Dialogue: 0,0:00:08.00,0:00:09.00,Default,,0,0,0,,q\xCC\xB8\n";

static void fail(const char *message)
{
    fprintf(stderr, "metrics test failed: %s\n", message);
    exit(1);
}

static uint64_t frame_fingerprint(const ASS_Image *image)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    while (image) {
        const uint32_t values[] = {
            image->w, image->h, image->stride, image->dst_x, image->dst_y,
            image->color, image->type,
        };
        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
            hash ^= values[i];
            hash *= UINT64_C(1099511628211);
        }
        for (int y = 0; y < image->h; y++) {
            for (int x = 0; x < image->w; x++) {
                hash ^= image->bitmap[y * image->stride + x];
                hash *= UINT64_C(1099511628211);
            }
        }
        image = image->next;
    }
    return hash;
}

int main(void)
{
    ASS_Library *library = ass_library_init();
    if (!library)
        fail("could not create library");

    ASS_Renderer *renderer = ass_renderer_init(library);
    if (!renderer)
        fail("could not create renderer");

    ass_set_frame_size(renderer, 1280, 720);
    ass_set_storage_size(renderer, 1280, 720);
    ass_set_fonts(renderer, TEST_FONT_PATH, NULL,
                  ASS_FONTPROVIDER_NONE, NULL, 0);

    ASS_Track *track = ass_read_memory(library, script, sizeof(script) - 1, NULL);
    if (!track)
        fail("could not parse track");

    ASS_Image *image = ass_render_frame(renderer, track, 500, NULL);
    if (!image)
        fail("active drawing did not produce an image");
    uint64_t fingerprint = frame_fingerprint(image);
    if (!ass_get_metrics(renderer, track, 500))
        fail("metrics failed between ordinary renders");
    image = ass_render_frame(renderer, track, 500, NULL);
    if (!image || frame_fingerprint(image) != fingerprint)
        fail("metrics collection changed ordinary rendering output");

    ASS_Metrics *metrics = ass_get_metrics(renderer, track, 500);
    if (!metrics || !metrics->event || !metrics->runs || !metrics->runs->fill)
        fail("active drawing did not produce metrics");
    if (metrics->next)
        fail("single event returned more than one metrics entry");

    if (ass_get_metrics(renderer, track, 1500))
        fail("inactive timestamp did not return NULL");
    if (ass_get_metrics(renderer, track, 1500))
        fail("repeated inactive timestamp did not return NULL");
    if (ass_get_metrics(renderer, track, 2500))
        fail("comment-only event did not return NULL");

    metrics = ass_get_metrics(renderer, track, 4500);
    if (!metrics || !metrics->next || metrics->next->next)
        fail("two active events did not produce a terminated two-entry list");

    // Rendering must release the metrics list even when no event is active.
    if (ass_render_frame(renderer, track, 1500, NULL))
        fail("inactive render unexpectedly produced images");

    metrics = ass_get_metrics(renderer, track, 500);
    if (!metrics || !metrics->runs)
        fail("metrics failed after an ordinary render call");

    metrics = ass_get_metrics(renderer, track, 6500);
    if (!metrics || !metrics->runs)
        fail("plain glyph did not produce metrics");
    double plain_advance = metrics->runs->advance.x;

    metrics = ass_get_metrics(renderer, track, 8500);
    if (!metrics || !metrics->runs)
        fail("combining cluster did not produce metrics");
    if (metrics->runs->advance.x != plain_advance)
        fail("combining glyph was counted twice in the run advance");

    ass_set_pixel_aspect(renderer, 2.0);
    metrics = ass_get_metrics(renderer, track, 6500);
    if (!metrics || !metrics->runs ||
            metrics->runs->advance.x != 2 * plain_advance)
        fail("pixel aspect ratio was not applied to the run advance");
    ass_set_pixel_aspect(renderer, 1.0);

    int event_count = track->n_events;
    ass_configure_prune(track, 0);
    metrics = ass_get_metrics(renderer, track, 8500);
    if (!metrics || !metrics->event)
        fail("pruning track did not produce metrics");
    if (track->n_events != event_count)
        fail("metrics collection invalidated its event pointers by pruning");
    if (!ass_render_frame(renderer, track, 8500, NULL))
        fail("ordinary render failed after metrics on a pruning track");
    if (track->n_events >= event_count)
        fail("ordinary rendering did not resume track pruning");

    ASS_Track *empty_track = ass_new_track(library);
    if (!empty_track)
        fail("could not create empty track");
    if (ass_get_metrics(renderer, empty_track, 0))
        fail("empty track did not return NULL");

    ass_free_track(empty_track);
    ass_free_track(track);
    ass_renderer_done(renderer);
    ass_library_done(library);
    return 0;
}
