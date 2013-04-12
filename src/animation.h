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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <weston/compositor.h>

#include "signal.h"
#include "animationcurve.h"

class ShellSurface;

class Animation {
public:
    enum class Flags {
        None = 0,
        SendDone = 1
    };
    Animation();
    Animation(const Animation &ani);
    ~Animation();

    void setStart(float value);
    void setTarget(float value);
    void run(struct weston_output *output, uint32_t duration, Flags flags = Flags::None);
    void stop();
    bool isRunning() const;
    void setCurve(const AnimationCurve &curve) { m_curve = curve.function(); }

    Animation &operator=(const Animation &ani);

    Signal<float> updateSignal;
    Signal<> doneSignal;

private:
    void update(struct weston_output *output, uint32_t msecs);

    struct AnimWrapper {
        struct weston_animation ani;
        Animation *parent;
    };
    AnimWrapper m_animation;
    float m_start;
    float m_target;
    uint32_t m_duration;
    uint32_t m_timestamp;
    Flags m_runFlags;
    AnimationCurve::Function m_curve;
    struct weston_output *m_output;
};

inline Animation::Flags operator|(Animation::Flags a, Animation::Flags b) {
    return (Animation::Flags)(a | b);
}

#endif
