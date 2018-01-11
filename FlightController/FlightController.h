/*
 *
 * Copyright (C) 2016 DMA <dma@ya.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#pragma once

#include <QMainWindow>

#include "../c2/c2_protocol.h"

#include "Delays.h"
#include "DeviceInterface.h"
#include "Hardware.h"
#include "FirmwareLoader.h"
#include "LayerConditions.h"
#include "LayoutEditor.h"
#include "MatrixMonitor.h"
#include "ThresholdEditor.h"

namespace Ui {
class FlightController;
}

class FlightController : public QMainWindow {
  Q_OBJECT
  Q_ENUMS(StatusPosition)

public:
  explicit FlightController(QWidget *parent = 0);
  ~FlightController();
  void setup(void);
  void show(void);
  LogViewer *getLogViewport(void);
  void setOldLogger(QtMessageHandler *logger);
  void logToViewport(const QString &);

signals:
  void sendCommand(c2command cmd, uint8_t msg);
  void flipStatusBit(deviceStatus bit);

public slots:
  void redButtonToggle(bool);
  void showKeyMonitor(void);
  void statusRequestButtonClick(void);
  void editLayoutClick(void);
  void editMacrosClick(void);
  void editThresholdsClick(void);
  void showLayerConditions(void);
  void mainTabChanged(int);
  void deviceStatusNotification(DeviceInterface::DeviceStatus);

protected:
  void closeEvent(QCloseEvent *);

private:
  Ui::FlightController *ui;
  MatrixMonitor *matrixMonitor;
  LayoutEditor *layoutEditor;
  ThresholdEditor *thresholdEditor;
  LayerConditions *layerConditions;
  Delays *_delays;
  Hardware *_hardware;
  FirmwareLoader *loader;
  QtMessageHandler *_oldLogger;
  bool _uiLocked = false;
  void lockUI(bool lock);
  void updateStatus(void);

private slots:
  void on_action_Setup_mode_triggered(bool bMode);
  void on_scanButton_clicked(void);
  void on_outputButton_clicked(void);
  void on_setupButton_clicked(void);
  void editDelays(void);
  void editExpHeader(void);
};
