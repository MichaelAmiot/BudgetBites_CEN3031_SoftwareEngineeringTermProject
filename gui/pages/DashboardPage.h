#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class DashboardPage; }
QT_END_NAMESPACE

class DashboardPage final : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override;

    Ui::DashboardPage* ui() const noexcept { return ui_; }

private:
    Ui::DashboardPage* ui_ = nullptr;
};
