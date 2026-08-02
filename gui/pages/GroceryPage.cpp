#include "GroceryPage.h"
#include "ui_GroceryPage.h"

GroceryPage::GroceryPage(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::GroceryPage) {
    ui_->setupUi(this);
}

GroceryPage::~GroceryPage() {
    delete ui_;
}
