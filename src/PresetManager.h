#pragma once

#include <QString>
#include <QVector>

namespace lumen {

// Encoder selection. Hardware encoders trade quality for CPU.
enum class Encoder {
    X264,        // libx264 (software, best quality/CPU ratio)
    NVENC_H264,  // NVIDIA hardware encoder
    QSV_H264,    // Intel QuickSync
    AMF_H264,    // AMD AMF
};

// Rate control mode. Twitch officially recommends CBR.
enum class RateControl {
    CBR,
    VBR,
    CQP,
};

// All settings that fully describe an outgoing stream. A Preset is just
// a named StreamConfig with the values fixed to Twitch's published
// guidelines so the user does not have to memorize them.
struct StreamConfig {
    int  widthPx       = 1920;
    int  heightPx      = 1080;
    int  fps           = 60;
    int  videoBitrateKbps = 6000;
    int  audioBitrateKbps = 160;
    int  audioSampleRateHz = 48000;
    int  keyframeIntervalSec = 2;     // Twitch requires 2s keyframes
    Encoder    encoder       = Encoder::X264;
    RateControl rateControl  = RateControl::CBR;
    QString    x264Preset    = QStringLiteral("veryfast"); // Twitch-recommended baseline
    QString    profile       = QStringLiteral("high");
};

struct Preset {
    QString id;            // stable identifier ("twitch-1080p60")
    QString displayName;   // localized label shown in UI ("Твич 1080p60")
    QString description;   // one-liner for tooltips
    StreamConfig config;
};

// Built-in Twitch presets sourced from
// https://help.twitch.tv/s/article/broadcasting-guidelines
class PresetManager {
public:
    static const QVector<Preset>& builtinPresets();
    static const Preset* findById(const QString& id);
    static const Preset* defaultPreset(); // "twitch-1080p60"
};

} // namespace lumen
