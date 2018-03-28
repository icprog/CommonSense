#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>

#include "../c2/c2_protocol.h"
#include "DeviceInterface.h"
#include "MacroEditor.h"
#include "singleton.h"
#include "ScancodeList.h"
#include "ui_MacroEditor.h"

namespace {
  const QString kNew{"-new-"};
}

MacroEditor::MacroEditor(DeviceConfig *config, QWidget *parent)
    : QFrame(parent), ui(new Ui::MacroEditor) {
  ui->setupUi(this);
  deviceConfig = config;
}

void MacroEditor::show(void) {
  ScancodeList scanCodes;
  ui->scanCode->addItems(scanCodes.list);
  ui->macroListCombo->clear();
  for (auto& m : deviceConfig->macros) {
    ui->macroListCombo->addItem(m.fullName());
  }
  ui->macroListCombo->addItem(kNew);
  ui->macroListCombo->setCurrentText(kNew);
  QWidget::show();
}

MacroEditor::~MacroEditor() { delete ui; }

void MacroEditor::on_applyButton_clicked() {
}

void MacroEditor::on_revertButton_clicked() {
  qInfo() << "Loaded Macros";
}

void MacroEditor::populateSteps(QByteArray &bytes) {
  int row = -1;
  int mptr = 0;
  while(mptr < bytes.size()) {
    addStep(++row);
    auto *cmd = qobject_cast<QComboBox *>(ui->bodyTable->cellWidget(row, 0));
    auto *delay = qobject_cast<QComboBox *>(ui->bodyTable->cellWidget(row, 1));
    auto *sc = qobject_cast<QComboBox *>(ui->bodyTable->cellWidget(row, 2));
    switch(bytes[mptr] >> 6) {
      case 0: // type
        cmd->setCurrentIndex(3);
        delay->setCurrentIndex(((uint8_t)bytes[mptr] >> 2) & 0x0f);
        sc->setCurrentIndex((uint8_t)bytes[++mptr]);
        break;
      case 1: // press or release
        if (bytes[mptr] & 0x20) {
          // key up
          cmd->setCurrentIndex(2);
        } else {
          // key down
          cmd->setCurrentIndex(1);
        }
        sc->setCurrentIndex((uint8_t)bytes[++mptr]);
        break;
    }
    ++mptr;
  }
}

void MacroEditor::on_macroListCombo_currentIndexChanged(int index) {
  bool existingMacro = true;
  ui->bodyTable->clearContents();
  ui->bodyTable->setRowCount(1);
  ui->bodyTable->setColumnCount(3);
  ui->bodyTable->setHorizontalHeaderLabels(
      QStringList() << "Command" << "Delay" << "Key");
  QPushButton *addStepButton = new QPushButton("+");
  connect(addStepButton, SIGNAL(clicked()), SLOT(addStepButtonClicked()));
  ui->bodyTable->setSpan(0, 0, 1, 3);
  ui->bodyTable->setCellWidget(0, 0, addStepButton);
  if (index + 1  == ui->macroListCombo->count()) {
    ui->scanCode->setCurrentIndex(0);
    ui->triggerEvent->setCurrentIndex(2);
    existingMacro = false;
  } else {
    auto& m = deviceConfig->macros[index];
    ui->scanCode->setCurrentIndex(m.keyCode);
    ui->triggerEvent->setCurrentText(m.getTriggerEventText());
    populateSteps(m.body);
  }
  ui->addButton->setDisabled(existingMacro);
  ui->deleteButton->setEnabled(existingMacro);
}

void MacroEditor::on_addButton_clicked() {
  auto pos = ui->macroListCombo->currentIndex();
  auto newMacro = Macro(ui->scanCode->currentIndex(),
                        ui->triggerEvent->currentText(),
                        QByteArray());
  for (auto m : deviceConfig->macros) {
    if (newMacro.keyCode == m.keyCode && newMacro.flags == m.flags) {
      QMessageBox::warning(this, "Error",
          "That activation sequence already taken!");
      return;
    }
  }
  deviceConfig->macros.push_back(newMacro);

  ui->macroListCombo->removeItem(pos);
  ui->macroListCombo->addItem(newMacro.fullName());
  ui->macroListCombo->addItem(kNew);
  ui->macroListCombo->setCurrentIndex(pos);
  on_macroListCombo_currentIndexChanged(pos);
}

void MacroEditor::on_deleteButton_clicked() {
  auto pos = ui->macroListCombo->currentIndex();
  ui->macroListCombo->removeItem(pos);
  deviceConfig->macros.erase(deviceConfig->macros.begin() + pos);
}

void MacroEditor::addStepButtonClicked() {
  auto row = ui->bodyTable->rowCount() - 1;
  addStep(row);
}

void MacroEditor::addStep(int row) {
  ui->bodyTable->insertRow(row);
  QComboBox *cmd = new QComboBox();
  cmd->addItems(QStringList{"-select-", "Press", "Release", "Type", "Wait"});
  connect(cmd, SIGNAL(currentIndexChanged(int)), SLOT(cmdIndexChanged(int)));
  ui->bodyTable->setCellWidget(row, 0, cmd);
  QComboBox *delay = new QComboBox();
  for (auto& d : deviceConfig->delays()) {
    delay->addItem(QString("%1 ms").arg(d));
  }
  ui->bodyTable->setCellWidget(row, 1, delay);
  QComboBox *sc = new QComboBox();
  sc->addItems(ScancodeList().list);
  ui->bodyTable->setCellWidget(row, 2, sc);
}

int MacroEditor::findWidgetRow(QWidget *w) {
  for(int i=0; i < ui->bodyTable->rowCount(); i++) {
    if (ui->bodyTable->cellWidget(i, 0) == w) {
      return i;
    }
  }
  return -1;
}

void MacroEditor::fillCommandParameters(int row, int command) {
  switch(command) {
    case 1:
      ui->bodyTable->cellWidget(row, 1)->setEnabled(false);
      ui->bodyTable->cellWidget(row, 2)->setEnabled(true);
      break;
    case 2:
      ui->bodyTable->cellWidget(row, 1)->setEnabled(false);
      ui->bodyTable->cellWidget(row, 2)->setEnabled(true);
      break;
    case 3:
      ui->bodyTable->cellWidget(row, 1)->setEnabled(true);
      ui->bodyTable->cellWidget(row, 2)->setEnabled(true);
      break;
    case 0:
    case 4:
      ui->bodyTable->cellWidget(row, 1)->setEnabled(true);
      ui->bodyTable->cellWidget(row, 2)->setEnabled(false);
  }
}

void MacroEditor::cmdIndexChanged(int idx) {
  auto *cb = qobject_cast<QComboBox *>(QObject::sender());
  if (!cb) {
    return;
  }
  int command = cb->currentIndex();
  int row = findWidgetRow(cb);
  if (row >= 0) {
    fillCommandParameters(row, command);
  }
}

void MacroEditor::on_closeButton_clicked() { this->close(); }
