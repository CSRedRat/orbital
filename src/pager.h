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

#ifndef PAGER_H
#define PAGER_H

#include <vector>

#include <weston/compositor.h>

#include "shell.h"

class Shell;
class Workspace;
class ShellSurface;
class Layer;

class Pager {
public:
    Pager(Shell *shell, Layer *layer);
    uint32_t addWorkspace();

    void selectWorkspace(uint32_t workspace);
    void selectNextWorkspace();
    void selectPreviousWorkspace();
    Workspace *currentWorkspace() const;
    Workspace *workspace(uint32_t workspace) const;
    uint32_t numWorkspaces() const;

    struct MoveGrab : public ShellGrab {
        Pager *pager;
    };
    virtual MoveGrab *moveSurface(ShellSurface *surface, struct weston_seat *seat);

protected:
    virtual void activateWorkspace();

    virtual void moveGrabMotion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    virtual void moveGrabButton(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w);

private:
    Shell *m_shell;
    Layer *m_parentLayer;
    std::vector<Workspace *> m_workspaces;
    int m_currentWorkspace;

    static void move_grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    static void move_grab_button(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w);
    static const struct wl_pointer_grab_interface m_move_grab_interface;
};

#endif
