#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include <emscripten/emscripten.h>

#include "apriltag.h"
#include "tag36h11.h"

#include "common/getopt.h"
#include "common/image_u8.h"
#include "common/image_u8x4.h"
#include "common/pjpeg.h"
#include "common/zarray.h"

apriltag_family_t *tf = NULL;
apriltag_detector_t *td = NULL;

EMSCRIPTEN_KEEPALIVE
int init() {
    tf = tag36h11_create();
    if (tf == NULL)
        return -1;

    td = apriltag_detector_create();
    if (td == NULL)
        return -1;

    apriltag_detector_add_family(td, tf);

    td->nthreads = 1;
    td->quad_decimate = 1.0;

    td->qtp.max_nmaxima = 10;
    td->qtp.min_cluster_pixels = 5;

    td->qtp.max_line_fit_mse = 10.0;
    td->qtp.cos_critical_rad = cos(10 * M_PI / 180);
    td->qtp.deglitch = 0;


    td->debug = 0;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int add_code(char code) {
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int set_detector_options(int range_thres, int refine_edges, int min_white_black_diff) {
    td->refine_edges = refine_edges;
    td->qtp.min_white_black_diff = min_white_black_diff;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int set_quad_decimate(float quad_decimate) {
    td->quad_decimate = quad_decimate;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int save_grayscale(uint8_t pixels[], uint8_t gray[], int cols, int rows) {
    const int len = cols*rows*4;
    for (int i = 0, j = 0; i < len; i+=4, j++) {
        gray[j] = pixels[i];
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int detect_tags(uint8_t gray[], int cols, int rows) {
    image_u8_t im = {
        .width = cols,
        .height = rows,
        .stride = cols,
        .buf = gray
    };

    zarray_t *detections = apriltag_detector_detect(td, &im);

    int sz = zarray_size(detections);

    for (int i = 0; i < sz; i++) {
        apriltag_detection_t *det;
        zarray_get(detections, i, &det);

        EM_ASM({
            const tag = {};

            tag["code"] = $0;

            tag["corners"] = [];

            const corner0 = {};
            corner0["x"] = $1;
            corner0["y"] = $2;
            tag["corners"].push(corner0);

            const corner1 = {};
            corner1["x"] = $3;
            corner1["y"] = $4;
            tag["corners"].push(corner1);

            const corner2 = {};
            corner2["x"] = $5;
            corner2["y"] = $6;
            tag["corners"].push(corner2);

            const corner3 = {};
            corner3["x"] = $7;
            corner3["y"] = $8;
            tag["corners"].push(corner3);

            const center = {};
            center["x"] = $9;
            center["y"] = $10;
            tag["center"] = center;

            const tagEvent = new CustomEvent("onGlitterTagFound", {detail: {tag: tag}});
            var scope;
            if ('function' === typeof importScripts)
                scope = self;
            else
                scope = window;
            scope.dispatchEvent(tagEvent);
        },
            det->id,
            det->p[0][0],
            det->p[0][1],
            det->p[1][0],
            det->p[1][1],
            det->p[2][0],
            det->p[2][1],
            det->p[3][0],
            det->p[3][1],
            det->c[0],
            det->c[1]
        );
    }
    apriltag_detections_destroy(detections);

    return sz;
}
