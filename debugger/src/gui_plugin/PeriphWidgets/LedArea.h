/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      LED's area renderer declaration.
 */

#pragma once

#include "api_core.h"   // MUST BE BEFORE QtWidgets.h or any other Qt header.
#include "attribute.h"
#include "igui.h"
#include "coreservices/isignal.h"
#include "coreservices/isignallistener.h"

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QtGui/QPaintEvent>

namespace debugger {

class LedArea : public QWidget {
    Q_OBJECT
public:
    LedArea(QWidget *parent = 0);

private slots:
    void slotUpdate(uint32_t val);

protected:
    void paintEvent(QPaintEvent *event_) Q_DECL_OVERRIDE;

private:
    uint32_t leds_;
    QPixmap pixmapBkg_;
    QPixmap pixmapGreen_;
    QPixmap pixmapGrey_;
};

}  // namespace debugger
