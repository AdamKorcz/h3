/*
 * Copyright 2017-2020 Uber Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include "h3Index.h"
#include "test.h"

#define PADDED_COUNT 10

static void verifyCountAndUniqueness(H3Index* children, int paddedCount,
                                     int expectedCount) {
    int numFound = 0;
    for (int i = 0; i < paddedCount; i++) {
        H3Index currIndex = children[i];
        if (currIndex == 0) {
            continue;
        }
        numFound++;
        // verify uniqueness
        int indexSeen = 0;
        for (int j = i + 1; j < paddedCount; j++) {
            if (children[j] == currIndex) {
                indexSeen++;
            }
        }
        t_assert(indexSeen == 0, "index was seen only once");
    }
    t_assert(numFound == expectedCount, "got expected number of children");
}

SUITE(cellToChildren) {
    GeoPoint sf = {0.659966917655, 2 * 3.14159 - 2.1364398519396};
    H3Index sfHex8;
    t_assertSuccess(H3_EXPORT(pointToCell)(&sf, 8, &sfHex8));

    TEST(oneResStep) {
        const int expectedCount = 7;
        const int paddedCount = 10;

        H3Index sfHex9s[PADDED_COUNT] = {0};
        H3_EXPORT(cellToChildren)(sfHex8, 9, sfHex9s);

        GeoPoint center;
        H3_EXPORT(cellToPoint)(sfHex8, &center);
        H3Index sfHex9_0;
        t_assertSuccess(H3_EXPORT(pointToCell)(&center, 9, &sfHex9_0));

        int numFound = 0;

        // Find the center
        for (int i = 0; i < paddedCount; i++) {
            if (sfHex9_0 == sfHex9s[i]) {
                numFound++;
            }
        }
        t_assert(numFound == 1, "found the center hex");

        // Get the neighbor hexagons by averaging the center point and outer
        // points then query for those independently

        CellBoundary outside;
        H3_EXPORT(cellToBoundary)(sfHex8, &outside);
        for (int i = 0; i < outside.numVerts; i++) {
            GeoPoint avg = {0};
            avg.lat = (outside.verts[i].lat + center.lat) / 2;
            avg.lon = (outside.verts[i].lon + center.lon) / 2;
            H3Index avgHex9;
            t_assertSuccess(H3_EXPORT(pointToCell)(&avg, 9, &avgHex9));
            for (int j = 0; j < expectedCount; j++) {
                if (avgHex9 == sfHex9s[j]) {
                    numFound++;
                }
            }
        }

        t_assert(numFound == expectedCount, "found all expected children");
    }

    TEST(multipleResSteps) {
        // Lots of children. Will just confirm number and uniqueness
        const int expectedCount = 49;
        const int paddedCount = 60;

        H3Index* children = calloc(paddedCount, sizeof(H3Index));
        H3_EXPORT(cellToChildren)(sfHex8, 10, children);

        verifyCountAndUniqueness(children, paddedCount, expectedCount);
        free(children);
    }

    TEST(sameRes) {
        const int expectedCount = 1;
        const int paddedCount = 7;

        H3Index* children = calloc(paddedCount, sizeof(H3Index));
        H3_EXPORT(cellToChildren)(sfHex8, 8, children);

        verifyCountAndUniqueness(children, paddedCount, expectedCount);
        free(children);
    }

    TEST(childResTooCoarse) {
        const int expectedCount = 0;
        const int paddedCount = 7;

        H3Index* children = calloc(paddedCount, sizeof(H3Index));
        H3_EXPORT(cellToChildren)(sfHex8, 7, children);

        verifyCountAndUniqueness(children, paddedCount, expectedCount);
        free(children);
    }

    TEST(childResTooFine) {
        const int expectedCount = 0;
        const int paddedCount = 7;
        H3Index sfHexMax;
        t_assertSuccess(H3_EXPORT(pointToCell)(&sf, MAX_H3_RES, &sfHexMax));

        H3Index* children = calloc(paddedCount, sizeof(H3Index));
        H3_EXPORT(cellToChildren)(sfHexMax, MAX_H3_RES + 1, children);

        verifyCountAndUniqueness(children, paddedCount, expectedCount);
        free(children);
    }

    TEST(pentagonChildren) {
        H3Index pentagon;
        setH3Index(&pentagon, 1, 4, 0);

        const int expectedCount = (5 * 7) + 6;
        const int paddedCount = H3_EXPORT(maxCellToChildrenSize)(pentagon, 3);

        H3Index* children = calloc(paddedCount, sizeof(H3Index));
        H3_EXPORT(cellToChildren)(sfHex8, 10, children);
        H3_EXPORT(cellToChildren)(pentagon, 3, children);

        verifyCountAndUniqueness(children, paddedCount, expectedCount);
        free(children);
    }
}
