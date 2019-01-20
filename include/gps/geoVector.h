/*
 * Race Capture Firmware
 *
 * Copyright (C) 2015 Autosport Labs
 *
 * This file is part of the Race Capture firmware suite
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details. You should
 * have received a copy of the GNU General Public License along with
 * this code. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEOVECTOR_H_
#define GEOVECTOR_H_

#include "cpp_guard.h"
#include "geopoint.h"

#include <stdbool.h>

CPP_GUARD_BEGIN

typedef struct _GeoVector {
        float x;
        float y;
        float z;
} GeoVector;

/**
 * Converts a vector given in ECEF to NED.
 * Based on Chapter 2 of 'Unmanned Rotorcraft Systems'
 * https://www.springer.com/gp/book/9780857296344
 */
int convertECEFToNED(GeoVector * ecef, const GeoPoint *p);

CPP_GUARD_END

#endif /* GEOVECTOR_H_ */
