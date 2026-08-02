#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class AccountPage; }
QT_END_NAMESPACE

class AccountPage final : public QWidget {
    Q_OBJECT

public:
    explicit AccountPage(QWidget* parent = nullptr);
    ~AccountPage() override;

    Ui::AccountPage* ui() const noexcept { return ui_; }

private:
    Ui::AccountPage* ui_ = nullptr;
};
