#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class PreferencesPage; }
QT_END_NAMESPACE

class PreferencesPage final : public QWidget {
    Q_OBJECT

public:
    explicit PreferencesPage(QWidget* parent = nullptr);
    ~PreferencesPage() override;

    Ui::PreferencesPage* ui() const noexcept { return ui_; }

private:
    Ui::PreferencesPage* ui_ = nullptr;
};
