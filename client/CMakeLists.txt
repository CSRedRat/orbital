
find_package(Cairo)

add_definitions(-DWESTON_DATADIR="${WESTON_PREFIX}/share/weston")
add_definitions(-DWESTON_BINDIR="${WESTON_PREFIX}/bin")

include_directories(${CAIRO_INCLUDE_DIRS})

set(SOURCES
    desktop-shell.c
    window.c
    cairo-util.c
    config-parser.c
    os-compatibility.c
    image-loader.c
        )

wayland_add_protocol_client(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
    desktop-shell
)

add_executable(orbital-shell-client ${SOURCES})
target_link_libraries(orbital-shell-client wayland-client wayland-cursor xkbcommon cairo m pixman-1 png15 jpeg)