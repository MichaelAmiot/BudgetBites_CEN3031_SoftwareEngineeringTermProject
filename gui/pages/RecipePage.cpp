#include "RecipePage.h"
#include "ui_RecipePage.h"

RecipePage::RecipePage(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::RecipePage) {
    ui_->setupUi(this);
}

RecipePage::~RecipePage() {
    delete ui_;
}
