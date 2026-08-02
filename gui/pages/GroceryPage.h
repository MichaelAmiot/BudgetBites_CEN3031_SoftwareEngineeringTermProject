#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class GroceryPage; }
QT_END_NAMESPACE

class GroceryPage final : public QWidget {
    Q_OBJECT

public:
    explicit GroceryPage(QWidget* parent = nullptr);
    ~GroceryPage() override;

    Ui::GroceryPage* ui() const noexcept { return ui_; }

private:
    Ui::GroceryPage* ui_ = nullptr;
};
