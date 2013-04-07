/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <weston/compositor.h>

#include "animation.h"
#include "signal.h"

class Transform {
public:
    Transform();

    void reset();
    void scale(float x, float y, float z);
    void translate(float x, float y, float z);

    void apply();
    void animate(struct weston_output *output, uint32_t duration);

    void currentTranslation(float *x, float *y = nullptr, float *z = nullptr) const;
    void currentScale(float *x, float *y = nullptr, float *z = nullptr) const;

    inline const struct weston_transform *nativeHandle() const { return &m_transform; }
    inline struct weston_transform *nativeHandle() { return &m_transform; }

    mutable Signal<> updatedSignal;

private:
    void updateAnim(float value);

    struct weston_transform m_transform;
    Animation m_animation;

    struct State {
        float scale[3];
        float translate[3];
    };

    State m_source;
    State m_current;
    State m_target;
};

#endif
