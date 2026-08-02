#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MealPlanPage; }
QT_END_NAMESPACE

class MealPlanPage final : public QWidget {
    Q_OBJECT

public:
    explicit MealPlanPage(QWidget* parent = nullptr);
    ~MealPlanPage() override;

    Ui::MealPlanPage* ui() const noexcept { return ui_; }

private:
    Ui::MealPlanPage* ui_ = nullptr;
};
