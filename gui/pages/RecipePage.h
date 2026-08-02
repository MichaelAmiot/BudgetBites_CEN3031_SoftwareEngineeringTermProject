#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class RecipePage; }
QT_END_NAMESPACE

class RecipePage final : public QWidget {
    Q_OBJECT

public:
    explicit RecipePage(QWidget* parent = nullptr);
    ~RecipePage() override;

    Ui::RecipePage* ui() const noexcept { return ui_; }

private:
    Ui::RecipePage* ui_ = nullptr;
};
