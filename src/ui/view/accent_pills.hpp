#ifndef __UI_VIEW_ACCENT_PILLS_HPP__
#define __UI_VIEW_ACCENT_PILLS_HPP__

#include "ui_theme.h"

/**
 * Puts the accent of the active source in the pill colour of the platform for its lifetime. It does
 * nothing while the "Source colours" setting is off. Wrap the platform calls that draw pills.
 */
class AccentPills {
public:
    AccentPills() : active(Theme_beginAccentPills()) {}
    ~AccentPills() {
        if (active) Theme_endAccentPills();
    }

    AccentPills(const AccentPills&)            = delete;
    AccentPills& operator=(const AccentPills&) = delete;

private:
    bool active;
};

#endif
