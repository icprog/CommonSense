#include <stdint.h>

#include <QLCDNumber>
#include <QPalette>
#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include "MatrixMonitor.h"
#include "ui_MatrixMonitor.h"
#include "DeviceInterface.h"
#include "singleton.h"
#include "Events.h"
#include "../c2/c2_protocol.h"

MatrixMonitor::MatrixMonitor(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::MatrixMonitor),
    debug(0), displayMode(0), grid(new QGridLayout())
{
    this->enableOutput(0); // tell the keyboard so it does not spam us before we install handler.
    ui->setupUi(this);
    connect(ui->VoltagesButton, SIGNAL(clicked()), this, SLOT(voltagesButtonClick()));
    connect(ui->CloseButton, SIGNAL(clicked()), this, SLOT(closeButtonClick()));
    connect(ui->ThresholdsButton, SIGNAL(clicked()), this, SLOT(thresholdsButtonClick()));
    connect(ui->ModeSelector, SIGNAL(currentTextChanged(QString)), this, SLOT(displayModeChanged(QString)));
    initDisplay();
    DeviceInterface &di = Singleton<DeviceInterface>::instance();
    connect(this, SIGNAL(sendCommand(uint8_t, uint8_t)), &di, SLOT(sendCommand(uint8_t, uint8_t)));
    di.installEventFilter(this);
    deviceConfig = di.getConfigPtr();
}

void MatrixMonitor::show(void)
{
    if (deviceConfig->matrixCols && deviceConfig->matrixCols)
    {
        updateDisplaySize(deviceConfig->matrixRows, deviceConfig->matrixCols);
        QWidget::show();
    } else
        QMessageBox::critical(this, "Error", "Matrix not configured - cannot monitor");
}

MatrixMonitor::~MatrixMonitor()
{
    delete ui;
}

void MatrixMonitor::initDisplay(void)
{
    for (uint8_t i = 1; i<=ABSOLUTE_MAX_COLS; i++)
    {
        grid->addWidget(new QLabel(QString("%1").arg(i)), 0, i, 1, 1, Qt::AlignRight);
        if (i <= ABSOLUTE_MAX_ROWS) {
            grid->addWidget(new QLabel(QString("%1").arg(i)), i, 0, 1, 1, Qt::AlignRight);
        }
    }
    for (uint8_t i = 0; i<ABSOLUTE_MAX_ROWS; i++)
    {
        for (uint8_t j = 0; j<ABSOLUTE_MAX_COLS; j++)
        {
            QLCDNumber *l = new QLCDNumber(3);
            l->setSegmentStyle(QLCDNumber::Filled);
            l->setMinimumHeight(25);
            l->setMaximumWidth(40);
            l->setVisible(false);
            grid->addWidget(l, i+1, j+1, 1, 1);
            display[i][j] = l;
        }
    }
    ui->Dashboard->setLayout(grid);
}

void MatrixMonitor::updateDisplaySize(uint8_t rows, uint8_t cols)
{
    for (uint8_t i = 1; i<=ABSOLUTE_MAX_COLS; i++)
    {
        if (i <= ABSOLUTE_MAX_ROWS)
            grid->itemAtPosition(i, 0)->widget()->setVisible(i<=rows);
        grid->itemAtPosition(0, i)->widget()->setVisible(i<=cols);

    }
    for (uint8_t i = 0; i<ABSOLUTE_MAX_ROWS; i++)
    {
        for (uint8_t j = 0; j<ABSOLUTE_MAX_COLS; j++)
        {
            QLCDNumber *l = display[i][j];
            l->setVisible((i < rows) & (j < cols));
        }
    }
    this->adjustSize();
}

bool MatrixMonitor::eventFilter(QObject *obj __attribute__((unused)), QEvent *event){
    if (event->type() == DeviceMessage::ET )
    {
        QByteArray *pl = static_cast<DeviceMessage *>(event)->getPayload();
        if (pl->at(0) != C2RESPONSE_MATRIX_STATUS)
            return false;
        uint8_t row = pl->at(1);
        uint8_t max_cols = pl->at(2);
        for (uint8_t i = 0; i<max_cols; i++) {
            QLCDNumber *cell = display[row][i];
            uint8_t level = pl->constData()[3+i];
            if (displayMode == 0
               or (displayMode == 1 and level < cell->intValue())
               or (displayMode == 2 and level > cell->intValue())
            )
                cell->display(level);
            /* TODO
            if (deviceConfig->capsenseFlags && level > deviceConfig->storage[row*deviceConfig->matrixCols + i])
                cell->setStyleSheet("background-color: #00ff00;");
            else if (!deviceConfig->capsense_flags.normallyOpen && level < deviceConfig->storage[row*deviceConfig->matrixCols + i])
                cell->setStyleSheet("background-color: #00ff00;");
            else
            */
                cell->setStyleSheet("background-color: #ffffff;");
        }
        return true;
    }
    return false;
}

void MatrixMonitor::enableOutput(uint8_t m)
{
    emit sendCommand(C2CMD_GET_MATRIX_STATE, m);
}

void MatrixMonitor::voltagesButtonClick(void)
{
    if (ui->VoltagesButton->text() == "Stop!") {
        ui->VoltagesButton->setText("Again!");
        enableOutput(0);
    } else {
        ui->VoltagesButton->setText("Stop!");
        enableOutput(1);
    }
}

void MatrixMonitor::thresholdsButtonClick(void)
{
    for (uint8_t i = 0; i<deviceConfig->matrixRows; i++)
    {
        for (uint8_t j = 0; j<deviceConfig->matrixCols; j++)
        {
            /* TODO
            if (deviceConfig->capsense_flags.normallyOpen)
            {
                deviceConfig->storage[i*deviceConfig->matrixCols + j] = display[i][j]->value() + ui->threshold->value();
            } else {
                deviceConfig->storage[i*deviceConfig->matrixCols + j] = display[i][j]->value() - ui->threshold->value();
            }
            */
        }
    }
}

void MatrixMonitor::closeButtonClick(void)
{
    this->close();
}

void MatrixMonitor::closeEvent (QCloseEvent *event)
{
    this->enableOutput(0);
    event->accept();
}

void MatrixMonitor::displayModeChanged(QString s)
{
    if (s == "Min")
        displayMode = 1;
    else if (s == "Max")
        displayMode = 2;
    else
        displayMode = 0;
}
