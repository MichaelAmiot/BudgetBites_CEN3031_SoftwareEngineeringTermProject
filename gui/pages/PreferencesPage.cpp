#include "PreferencesPage.h"
#include "ui_PreferencesPage.h"

PreferencesPage::PreferencesPage(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::PreferencesPage) {
    ui_->setupUi(this);
}

PreferencesPage::~PreferencesPage() {
    delete ui_;
}
