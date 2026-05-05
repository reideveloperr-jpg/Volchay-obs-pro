#include "MainWindow.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QLocale>
#include <QStringLiteral>

int main(int argc, char* argv[]) {
    QApplication::setOrganizationName(QStringLiteral("LumenStream"));
    QApplication::setOrganizationDomain(QStringLiteral("lumen-stream.local"));
    QApplication::setApplicationName(QStringLiteral("Lumen Stream"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QApplication app(argc, argv);

    lumen::ThemeManager theme(&app);

    lumen::MainWindow w(&theme);
    w.show();
    return app.exec();
}
