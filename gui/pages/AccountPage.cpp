#include "AccountPage.h"
#include "ui_AccountPage.h"

AccountPage::AccountPage(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::AccountPage) {
    ui_->setupUi(this);
}

AccountPage::~AccountPage() {
    delete ui_;
}
