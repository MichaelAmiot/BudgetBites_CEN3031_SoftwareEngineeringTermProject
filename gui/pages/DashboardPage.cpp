#include "DashboardPage.h"
#include "ui_DashboardPage.h"

DashboardPage::DashboardPage(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::DashboardPage) {
    ui_->setupUi(this);
}

DashboardPage::~DashboardPage() {
    delete ui_;
}
