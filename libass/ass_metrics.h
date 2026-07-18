/*
 * Copyright (C) 2024 libass contributors
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

#ifndef LIBASS_METRICS_H
#define LIBASS_METRICS_H

#include <stddef.h>

#include "ass_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * API to obtain text metrics and shape data for subtitles.
 * This API is intended to ease the authoring of subtitles by providing
 * a unified and platform-independent method to obtain metrics.
 * However, there are no guarantees for the stability of the data returned by
 * this API. With future changes to libass and its rendering, the returned
 * metrics and shape data may change too.
 *
 * For example, subtitle authors could use this API to determine when and
 * where libass automatically breaks a subtitle line into two, but there is
 * no guarantee that newer or older versions of libass (as well as other
 * renderers) will always break this line in the same way. Subtitle authors
 * that want to ensure that a line is broken in a certain way should instead
 * use manual line breaks and overrides (e.g. \q2).
 *
 * API Usage:
 * After initializing and configuring an ASS_Renderer and ASS_Track in the
 * same way as one would for normal subtitle rendering, users may call
 * ass_get_metrics to obtain metrics for all events rendered at the given
 * point in time.
 * This returns a list of event metrics, each of which consisting of a list
 * of runs, with metrics for those runs. A run contains lists of shapes
 * which, when drawn together, make up the shape for the fill or border of
 * the run.
 * The individual shapes correspond to individual glyphs rendered in the run,
 * but due to font shaping there is no guarantee for these glyphs to
 * correspond one-to-one to characters in the run's text.
 *
 * Positions, advances, ascenders, descenders, and outline points are measured
 * in output-frame pixels. Run positions use the output-frame origin. Outline
 * points are relative to the position of their run.
 */


/*
 * An outline is represented by an array of points and an array of segments.
 * Segments can be splines of order 1 (line), 2 (quadratic) or 3 (cubic).
 * Each segment owns a number of points equal to its order in the point array
 * and uses the first point owned by the next segment as its last point.
 * The last segment in each contour uses the first point owned by the first
 * segment in each contour as its last point.
 * Correspondingly, the total number of points is equal to the sum of the
 * spline orders of all segments.
 */

enum {
    ASS_METRICS_OUTLINE_LINE_SEGMENT     = 1,  // line segment
    ASS_METRICS_OUTLINE_QUADRATIC_SPLINE = 2,  // quadratic spline
    ASS_METRICS_OUTLINE_CUBIC_SPLINE     = 3,  // cubic spline
    ASS_METRICS_OUTLINE_COUNT_MASK       = 3,  // spline order mask
    ASS_METRICS_OUTLINE_CONTOUR_END      = 4   // last segment in contour flag
};

typedef struct ass_metrics_outline {
    size_t n_points;
    size_t n_segments;
    ASS_DVector *points;  // NULL if n_points is zero
    char *segments;       // NULL if n_segments is zero

    struct ass_metrics_outline *next;   // Next glyph outline, or NULL
} ASS_Metrics_Outline;

/* Metrics and shape data for one visually uniform run on one line. */
typedef struct ass_run_metrics {
    ASS_DVector pos;      // Baseline origin in output-frame pixels
    ASS_DVector advance;  // Shaped advance of all clusters in the run
    double asc;           // Maximum ascender above the baseline
    double desc;          // Maximum descender below the baseline

    ASS_Metrics_Outline *fill;    // Glyph fill outlines relative to pos
    ASS_Metrics_Outline *border;  // Glyph border outlines relative to pos

    struct ass_run_metrics *next;    // Next run, or NULL
} ASS_RunMetrics;

/* ASS_Metrics is a set of metrics corresponding to a single event. */
typedef struct ass_metrics {
    ASS_Event *event;   // Becomes invalid once the containing ASS_Track is modified, pruned, or freed
    ASS_RunMetrics *runs;
    struct ass_metrics *next;   // Next set of metrics, or NULL
} ASS_Metrics;

/**
 * \brief Get metrics for a frame, producing a list of ASS_Metrics.
 * Returns NULL if no event produces metrics, frame setup fails, or memory
 * allocation fails. The returned list and all data it owns live until the next
 * call to ass_get_metrics or ass_render_frame, or until ass_renderer_done.
 * The caller must not free or modify any part of the list.
 *
 * \param priv renderer handle
 * \param track subtitle track
 * \param now video timestamp in milliseconds
 */
ASS_Metrics *ass_get_metrics(ASS_Renderer *priv, ASS_Track *track, long long now);

#ifdef __cplusplus
}
#endif

#endif /* LIBASS_METRICS_H */
