#include "MealPlanPage.h"
#include "ui_MealPlanPage.h"

MealPlanPage::MealPlanPage(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::MealPlanPage) {
    ui_->setupUi(this);
}

MealPlanPage::~MealPlanPage() {
    delete ui_;
}
