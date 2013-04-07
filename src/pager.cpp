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

#include "pager.h"
#include "workspace.h"
#include "shell.h"
#include "shellsurface.h"

#include "wayland-desktop-shell-server-protocol.h"

static const int SLIDE_DURATION = 300;

Pager::Pager(Shell *shell, Layer *layer)
     : m_shell(shell)
     , m_parentLayer(layer)
     , m_currentWorkspace(0)
{
    addWorkspace();
}

uint32_t Pager::addWorkspace()
{
    Workspace *ws = new Workspace(m_shell, m_workspaces.size());
    m_workspaces.push_back(ws);
    ws->insert(m_parentLayer);
    activateWorkspace();

    return ws->number();
}

void Pager::selectWorkspace(uint32_t workspace)
{
    if (workspace >= m_workspaces.size()) {
        return;
    }

    m_currentWorkspace = workspace;
    activateWorkspace();
}

void Pager::selectNextWorkspace()
{
    if (++m_currentWorkspace == (int)m_workspaces.size()) {
        m_currentWorkspace = 0;
    }
    activateWorkspace();
}

void Pager::selectPreviousWorkspace()
{
    if (--m_currentWorkspace < 0) {
        m_currentWorkspace = m_workspaces.size() - 1;
    }
    activateWorkspace();
}

Workspace *Pager::currentWorkspace() const
{
    return m_workspaces[m_currentWorkspace];
}

Workspace *Pager::workspace(uint32_t ws) const
{
    if (ws >= m_workspaces.size()) {
        return nullptr;
    }

    return m_workspaces[ws];
}

uint32_t Pager::numWorkspaces() const
{
    return m_workspaces.size();
}

void Pager::activateWorkspace()
{
    int numWs = numWorkspaces();
    int numWsCols = ceil(sqrt(numWs));

    int currWsCol = m_currentWorkspace % numWsCols;
    int currWsRow = m_currentWorkspace / numWsCols;
    int off_x = currWsCol * currentWorkspace()->output()->width;
    int off_y = currWsRow * currentWorkspace()->output()->height;

    for (uint i = 0; i < m_workspaces.size(); ++i) {
        Workspace *w = m_workspaces[i];

        int cws = i % numWsCols;
        int rws = i / numWsCols;

        Transform tr = w->transform();
        tr.reset();
        tr.translate(cws * w->output()->width - off_x, rws * w->output()->height - off_y, 0.f);
        tr.animate(w->output(), SLIDE_DURATION);
        w->setTransform(tr);

    }
}

// ---- Move stuff

struct PgMoveGrab : public Pager::MoveGrab {
    struct ShellSurface *shsurf;
    struct wl_listener shsurf_destroy_listener;
    wl_fixed_t dx, dy;
    uint32_t touchedEdgeTime;
    Transform transform;
    int x, y;
    int delta_x, delta_y;

    void update()
    {
        float x, y;
        transform.currentTranslation(&x, &y);

        struct weston_seat *seat = container_of(pointer->seat, struct weston_seat, seat);
        struct weston_surface *sprite = seat->sprite;
        if (sprite) {
            weston_surface_set_position(seat->sprite, delta_x + (int)x - seat->hotspot_x, delta_y + (int)y - seat->hotspot_y);
            weston_surface_schedule_repaint(seat->sprite);
        }

        shsurf->setPosition(delta_x + (int)x + wl_fixed_to_int(dx), delta_y + (int)y + wl_fixed_to_int(dy));
        shsurf->repaint();
    }
};

void Pager::moveGrabMotion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    ShellGrab *shgrab = container_of(grab, ShellGrab, grab);
    PgMoveGrab *move = static_cast<PgMoveGrab *>(shgrab);

    struct wl_pointer *pointer = grab->pointer;
    ShellSurface *shsurf = move->shsurf;
    int dx = wl_fixed_to_int(pointer->x + move->dx);
    int dy = wl_fixed_to_int(pointer->y + move->dy);

    if (!shsurf)
        return;

    int ptx = wl_fixed_to_int(pointer->x);
    struct weston_output *out = shsurf->output();
    int changeWs = 0;
    if (ptx >= out->width - 1) {
        changeWs = 1;
    } else if (ptx <= 0) {
        changeWs = -1;
    }
    if (changeWs != 0) {
        if (move->touchedEdgeTime == 0) {
            move->touchedEdgeTime = time;
        } else if (time - move->touchedEdgeTime > 200) {
            if (changeWs == 1) {
                move->shell->pager()->selectNextWorkspace();
                notify_motion(container_of(pointer->seat, struct weston_seat, seat), time, -wl_fixed_from_int(out->width - 5), 0);
                move->transform.translate(out->width - 5, wl_fixed_to_int(pointer->y), 0);
                move->transform.apply();
                move->transform.translate(0, wl_fixed_to_int(pointer->y), 0);

                move->x = 0;
            } else {
                move->shell->pager()->selectPreviousWorkspace();
                notify_motion(container_of(pointer->seat, struct weston_seat, seat), time, wl_fixed_from_int(out->width - 5), 0);
                move->transform.translate(0, wl_fixed_to_int(pointer->y), 0);
                move->transform.apply();
                move->transform.translate(out->width - 5, wl_fixed_to_int(pointer->y), 0);

                move->x = out->width - 5;
            }
            move->transform.animate(out, SLIDE_DURATION);
            move->update();

            move->touchedEdgeTime = 0;

            move->y = wl_fixed_to_int(pointer->y);
            move->delta_x = 0;
            move->delta_y = 0;
            return;
        }
    }

    move->delta_x = wl_fixed_to_int(pointer->x) - move->x;
    move->delta_y = wl_fixed_to_int(pointer->y) - move->y;
    shsurf->setPosition(dx, dy);

    weston_compositor_schedule_repaint(shsurf->compositor());
}

void Pager::moveGrabButton(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shell_grab = container_of(grab, ShellGrab, grab);
    PgMoveGrab *move = static_cast<PgMoveGrab *>(shell_grab);
    struct wl_pointer *pointer = grab->pointer;
    enum wl_pointer_button_state state = (wl_pointer_button_state)state_w;

    if (pointer->button_count == 0 && state == WL_POINTER_BUTTON_STATE_RELEASED) {
        Shell::endGrab(shell_grab);
        move->shell->pager()->currentWorkspace()->addSurface(move->shsurf);
        move->shsurf->moveEndSignal(move->shsurf);
        delete move;
    }
}

void Pager::move_grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    ShellGrab *shell_grab = container_of(grab, ShellGrab, grab);
    static_cast<Pager::MoveGrab *>(shell_grab)->pager->moveGrabMotion(grab, time, x, y);
}

void Pager::move_grab_button(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shell_grab = container_of(grab, ShellGrab, grab);
    static_cast<Pager::MoveGrab *>(shell_grab)->pager->moveGrabButton(grab, time, button, state_w);
}

const struct wl_pointer_grab_interface Pager::m_move_grab_interface = {
    [](struct wl_pointer_grab *grab, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {},
    Pager::move_grab_motion,
    Pager::move_grab_button,
};

Pager::MoveGrab *Pager::moveSurface(ShellSurface *surface, struct weston_seat *seat)
{
    PgMoveGrab *move = new PgMoveGrab;

    move->pager = this;
    move->dx = wl_fixed_from_double(surface->x()) - seat->seat.pointer->grab_x;
    move->dy = wl_fixed_from_double(surface->y()) - seat->seat.pointer->grab_y;
    move->shsurf = surface;
    move->grab.focus = surface->wl_surface();
    move->touchedEdgeTime = 0;
    move->transform.updatedSignal.connect(move, &PgMoveGrab::update);

    m_shell->startGrab(move, &m_move_grab_interface, seat, DESKTOP_SHELL_CURSOR_MOVE);
    surface->workspace()->removeSurface(surface);
    m_shell->putInLimbo(surface);

    return move;
}
