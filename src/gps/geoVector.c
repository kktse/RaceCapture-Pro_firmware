/*
 * Race Capture Firmware
 *
 * Copyright (C) 2016 Autosport Labs
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


#include "geoVector.h"
#include "geopoint.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Converts a given value to radians.
 * @param val The provided value in non-radian form.
 * @return The radian value.
 */
static float toRad(float val)
{
        return val * (M_PI / 180.0);
}

int convertECEFToNED(GeoVector * ecef, const GeoPoint *p)
{
        const float sinLat = sin(toRad(p->latitude));
        const float sinLon = sin(toRad(p->longitude));
        const float cosLat = cos(toRad(p->latitude));
        const float cosLon = cos(toRad(p->longitude));

        const float x = - sinLat * cosLon * ecef->x
                        - sinLat * sinLon * ecef->y
                        + cosLat * ecef->z;
        const float y = - sinLon * ecef->x
                        + cosLon * ecef->y;
        const float z = - cosLat * cosLon * ecef->x
                        - cosLat * cosLon * ecef->y
                        - sinLat * ecef->z;

        ecef->x = x;
        ecef->y = y;
        ecef->z = z;

        return 0;
}
