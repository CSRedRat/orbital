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

#include "transform.h"

#include <weston/matrix.h>

Transform::Transform()
{
    wl_list_init(&m_transform.link);
    reset();
    m_source = { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 0.f } };
    m_current = m_target = m_source;

    m_animation.updateSignal.connect(this, &Transform::updateAnim);
}

void Transform::reset()
{
    weston_matrix_init(&m_transform.matrix);
    m_target = { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 0.f } };
}

void Transform::scale(float x, float y, float z)
{
    weston_matrix_scale(&m_transform.matrix, x, y, z);
    m_target.scale[0] = x;
    m_target.scale[1] = y;
    m_target.scale[2] = z;
}

void Transform::translate(float x, float y, float z)
{
    weston_matrix_translate(&m_transform.matrix, x, y, z);
    m_target.translate[0] = x;
    m_target.translate[1] = y;
    m_target.translate[2] = z;
}

void Transform::apply()
{
    m_current = m_target;
}

void Transform::animate(struct weston_output *output, uint32_t duration)
{
    struct weston_matrix *matrix = &m_transform.matrix;
    weston_matrix_init(matrix);
    weston_matrix_scale(&m_transform.matrix, m_current.scale[0], m_current.scale[1], m_current.scale[2]);
    weston_matrix_translate(&m_transform.matrix, m_current.translate[0], m_current.translate[1], m_current.translate[2]);

    m_source = m_current;
    m_animation.setStart(0.f);
    m_animation.setTarget(1.f);
    m_animation.run(output, duration);
}

void Transform::currentTranslation(float *x, float *y, float *z) const
{
    if (x)
        *x = m_current.translate[0];
    if (y)
        *y = m_current.translate[1];
    if (z)
        *z = m_current.translate[2];
}

void Transform::currentScale(float *x, float *y, float *z) const
{
    if (x)
        *x = m_current.scale[0];
    if (y)
        *y = m_current.scale[1];
    if (z)
        *z = m_current.scale[2];
}

void Transform::updateAnim(float value)
{
    struct weston_matrix *matrix = &m_transform.matrix;
    weston_matrix_init(matrix);

    for (int i = 0; i < 3; ++i) {
        m_current.scale[i] = m_source.scale[i] + (m_target.scale[i] - m_source.scale[i]) * value;
    }
    weston_matrix_scale(matrix, m_current.scale[0], m_current.scale[1], m_current.scale[2]);

    for (int i = 0; i < 3; ++i) {
        m_current.translate[i] = m_source.translate[i] + (m_target.translate[i] - m_source.translate[i]) * value;
    }
    weston_matrix_translate(matrix, m_current.translate[0], m_current.translate[1], m_current.translate[2]);

    updatedSignal();
}
