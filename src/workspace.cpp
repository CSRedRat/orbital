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

#include "workspace.h"
#include "shell.h"
#include "shellsurface.h"

Workspace::Workspace(Shell *shell, int number)
         : m_shell(shell)
         , m_number(number)
{
    int x = 0, y = 0;
    int w = 1, h = 1; // cannot use 0 here or else the transforms won't be calculated

    m_rootSurface = weston_surface_create(shell->compositor());
    m_rootSurface->configure = [](struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height) {};
    m_rootSurface->configure_private = 0;
    weston_surface_configure(m_rootSurface, x, y, w, h);
    weston_surface_set_color(m_rootSurface, 0.0, 0.0, 0.0, 0.0);
    pixman_region32_fini(&m_rootSurface->opaque);
    pixman_region32_init_rect(&m_rootSurface->opaque, 0, 0, 0, 0);
    pixman_region32_fini(&m_rootSurface->input);
    pixman_region32_init_rect(&m_rootSurface->input, 0, 0, w, h);

    m_layer.addSurface(m_rootSurface);
}

Workspace::~Workspace()
{

}

float Workspace::x() const
{
    pixman_box32_t *box = pixman_region32_extents(&m_rootSurface->transform.boundingbox);
    return box->x1;
}

float Workspace::y() const
{
    pixman_box32_t *box = pixman_region32_extents(&m_rootSurface->transform.boundingbox);
    return box->y1;
}

void Workspace::addSurface(ShellSurface *surface)
{
    if (!surface->transformParent()) {
        weston_surface_set_transform_parent(surface->m_surface, m_rootSurface);
    }
    m_layer.addSurface(surface);
    surface->m_workspace = this;
}

void Workspace::removeSurface(ShellSurface *surface)
{
    if (surface->transformParent() == m_rootSurface) {
        weston_surface_set_transform_parent(surface->m_surface, nullptr);
    }
    wl_list_remove(&surface->m_surface->layer_link);
    surface->m_workspace = nullptr;
}

void Workspace::restack(ShellSurface *surface)
{
    m_layer.restack(surface);
}

void Workspace::stackAbove(struct weston_surface *surf, struct weston_surface *parent)
{
    m_layer.stackAbove(surf, parent);
}

void Workspace::configure_background_surface(struct weston_surface *es, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width == 0)
        return;

    Workspace *ws = static_cast<Workspace *>(es->configure_private);

    for (struct weston_surface *s: ws->m_background) {
        if (s->output == es->output && s != es) {
            weston_surface_unmap(s);
            s->configure = NULL;
        }
    }

    weston_surface_configure(es, es->output->x, es->output->y, width, height);

    if (wl_list_empty(&es->layer_link)) {
        ws->m_background.addSurface(es);
        weston_compositor_schedule_repaint(es->compositor);
    }
}

void Workspace::setBackground(struct weston_surface *surface)
{
    m_backgroundSurface = surface;
    weston_surface_set_transform_parent(surface, m_rootSurface);
    surface->configure = configure_background_surface;
    surface->configure_private = this;
}

void Workspace::setTransform(const Transform &tr)
{
    wl_list_remove(&m_transform.nativeHandle()->link);
    m_transform = tr;
    m_transform.updatedSignal.connect(this, &Workspace::damage);

    wl_list_insert(&m_rootSurface->geometry.transformation_list, &m_transform.nativeHandle()->link);

    damage();
}

int Workspace::numberOfSurfaces() const
{
    return m_layer.numberOfSurfaces();
}

struct weston_output *Workspace::output() const
{
    return m_shell->getDefaultOutput();
    return m_rootSurface->output;
}

void Workspace::insert(Workspace *ws)
{
    m_layer.insert(&ws->m_layer);
    m_background.insert(&m_layer);
}

void Workspace::insert(Layer *layer)
{
    m_layer.insert(layer);
    m_background.insert(&m_layer);
}

void Workspace::insert(struct weston_layer *layer)
{
    m_layer.insert(layer);
    m_background.insert(&m_layer);
}

void Workspace::remove()
{
    m_layer.remove();
}

void Workspace::clip()
{
    float x, y;
    m_transform.currentTranslation(&x, &y);
    IRect2D rect;
    rect.x = x;
    rect.y = y;
    m_transform.currentScale(&x, &y);
    rect.width = output()->width * x;
    rect.height = output()->height * y;

    m_layer.clip(rect);
}

void Workspace::damage()
{
    clip();

    weston_surface_geometry_dirty(m_rootSurface);
    weston_surface_damage(m_rootSurface);
}
