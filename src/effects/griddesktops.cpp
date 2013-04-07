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

#include <linux/input.h>

#include "griddesktops.h"
#include "shellsurface.h"
#include "shell.h"
#include "animation.h"
#include "workspace.h"
#include "transform.h"

#include "wayland-desktop-shell-server-protocol.h"

struct Grab : public ShellGrab {
    GridDesktops *effect;
    ShellSurface *surface;
    bool moving;
    Transform surfTransform;
    wl_fixed_t dx, dy;
    float scale;
};

void GridDesktops::grab_focus(struct wl_pointer_grab *base, struct wl_surface *surf, wl_fixed_t x, wl_fixed_t y)
{

}

static void grab_motion(struct wl_pointer_grab *base, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    ShellGrab *shgrab = container_of(base, ShellGrab, grab);
    Grab *grab = static_cast<Grab *>(shgrab);

    if (grab->surface) {
        int pos_x = wl_fixed_to_int(base->pointer->x + grab->dx);
        int pos_y = wl_fixed_to_int(base->pointer->y + grab->dy);
        if (!grab->moving) {
            int dx = pos_x - grab->surface->x();
            int dy = pos_y - grab->surface->y();
            if (fabsf(dx) + fabsf(dy) < 5) {
                return;
            }
            grab->moving = true;

            grab->surface->workspace()->removeSurface(grab->surface);
            grab->shell->putInLimbo(grab->surface);
            grab->surfTransform.reset();
            grab->surfTransform.scale(grab->scale, grab->scale, 1.f);

            grab->surface->setPosition(grab->surface->transformedX(), grab->surface->transformedY());
            grab->surface->addTransform(grab->surfTransform);
        }

        grab->surface->setPosition(pos_x, pos_y);
    }
}

void GridDesktops::grab_button(struct wl_pointer_grab *base, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shgrab = container_of(base, ShellGrab, grab);
    Grab *grab = static_cast<Grab *>(shgrab);

    int numWs = grab->shell->numWorkspaces();
    int numWsCols = ceil(sqrt(numWs));
    int numWsRows = ceil((float)numWs / (float)numWsCols);

    struct weston_output *out = grab->shell->getDefaultOutput();
    int cellW = out->width / numWsCols;
    int cellH = out->height / numWsRows;

    int c = wl_fixed_to_int(base->pointer->x) / cellW;
    int r = wl_fixed_to_int(base->pointer->y) / cellH;
    int ws = r * numWsCols + c;

    if (state_w == WL_POINTER_BUTTON_STATE_PRESSED) {
        struct weston_surface *surface = (struct weston_surface *) base->pointer->current;
        ShellSurface *shsurf = grab->shell->getShellSurface(surface);
        if (shsurf) {
            grab->dx = wl_fixed_from_double(shsurf->transformedX()) - base->pointer->grab_x;
            grab->dy = wl_fixed_from_double(shsurf->transformedY()) - base->pointer->grab_y;
            grab->surface = shsurf;
            grab->moving = false;
        }
    } else {
        if (grab->surface && grab->moving) {
            grab->surface->removeTransform(grab->surfTransform);
            Workspace *w = grab->shell->workspace(ws);
            w->addSurface(grab->surface);

            float dx = wl_fixed_to_int(base->pointer->x + grab->dx);
            float dy = wl_fixed_to_int(base->pointer->y + grab->dy);
            grab->surface->setPosition((int)((dx - w->x()) / grab->scale) , (int)((dy - w->y()) / grab->scale));
        } else {
            grab->effect->m_setWs = ws;
            grab->effect->run(grab->effect->m_seat);
        }
        grab->surface = nullptr;
    }
}

const struct wl_pointer_grab_interface GridDesktops::grab_interface = {
    GridDesktops::grab_focus,
    grab_motion,
    GridDesktops::grab_button,
};

GridDesktops::GridDesktops(Shell *shell)
           : Effect(shell)
           , m_scaled(false)
           , m_grab(new Grab)
{
    m_grab->effect = this;
    m_grab->surface = nullptr;
    m_binding = shell->bindKey(KEY_G, MODIFIER_CTRL, &GridDesktops::run, this);
}

GridDesktops::~GridDesktops()
{
    delete m_binding;
    delete m_grab;
}

void GridDesktops::run(struct wl_seat *seat, uint32_t time, uint32_t key)
{
    run((struct weston_seat *)seat);
}

void GridDesktops::run(struct weston_seat *ws)
{
    if (shell()->isInFullscreen()) {
        return;
    }

    int numWs = shell()->numWorkspaces();
    int numWsCols = ceil(sqrt(numWs));
    int numWsRows = ceil((float)numWs / (float)numWsCols);

    if (m_scaled) {
        shell()->showPanels();
        shell()->endGrab(m_grab);
        shell()->selectWorkspace(m_setWs);
    } else {
        shell()->hidePanels();
        shell()->startGrab(m_grab, &grab_interface, ws, DESKTOP_SHELL_CURSOR_ARROW);
        m_setWs = shell()->currentWorkspace()->number();
        struct weston_output *out = shell()->currentWorkspace()->output();

        const int margin_w = out->width / 70;
        const int margin_h = out->height / 70;

        float rx = (1.f - (1 + numWsCols) * margin_w / (float)out->width) / (float)numWsCols;
        float ry = (1.f - (1 + numWsRows) * margin_h / (float)out->width) / (float)numWsRows;
        if (rx > ry) {
            rx = ry;
        } else {
            ry = rx;
        }
        m_grab->scale = rx;

        for (int i = 0; i < numWs; ++i) {
            Workspace *w = shell()->workspace(i);

            int cws = i % numWsCols;
            int rws = i / numWsCols;

            int x = cws * (out->width - margin_w * (1 + numWsCols)) / numWsCols + (1 + cws) * margin_w;
            int y = rws * (out->height - margin_h * (1 + numWsRows)) / numWsRows + (1 + rws) * margin_h;

            Transform tr = w->transform();
            tr.scale(m_grab->scale, m_grab->scale, 1);
            tr.translate(x, y, 0);
            tr.animate(out, 300);
            w->setTransform(tr);
        }
    }
    m_scaled = !m_scaled;
}
