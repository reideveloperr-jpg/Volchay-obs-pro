#pragma once

#include <QList>
#include <QString>

namespace lumen {

// One entry per attached display, in the order Qt reports them. The
// platform-specific input arg is what we hand to ffmpeg.
struct ScreenInfo {
    int     index = 0;       // index into QGuiApplication::screens()
    QString label;           // human-readable, e.g. "Display 1 — 2560×1440"
    int     x = 0, y = 0;    // top-left of the screen in virtual desktop
    int     w = 0, h = 0;    // pixel size
};

// One entry per audio capture device the OS exposes. `id` is what the
// platform's ffmpeg backend wants (dshow device name, PulseAudio source
// name, avfoundation index). `label` is a UI string and may include the
// id verbatim if the platform doesn't expose a friendlier name.
struct AudioDevice {
    QString id;
    QString label;
};

QList<ScreenInfo>  enumerateScreens();
QList<AudioDevice> enumerateMicrophones();
QList<AudioDevice> enumerateDesktopAudio();

} // namespace lumen
