/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2015                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __TIMECONTROLWIDGET_H__
#define __TIMECONTROLWIDGET_H__

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

class TimeControlWidget : public QWidget {
Q_OBJECT
public:
	TimeControlWidget(QWidget* parent);

    void update(QString currentTime, QString currentDelta);

signals:
    void scriptActivity(QString script);

private slots:
    void onValueChange();
    void onRewindButton();
    void onPauseButton();
    void onPlayButton();
    void onForwardButton();

private:
    QLabel* _currentTime;
    QComboBox* _setTime;
    QLabel* _currentDelta;
    QSlider* _setDelta;
    QPushButton* _rewind;
    QPushButton* _pause;
    QPushButton* _play;
    QPushButton* _forward;

    bool _stateNoNotification = false;
};

#endif // __TIMECONTROLWIDGET_H__