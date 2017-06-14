// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// -------------------------------------------------

#ifndef COLORPICKERDIALOG_H
#define COLORPICKERDIALOG_H

#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QLineEdit>
#include <QColorDialog>
#include <QDebug>
#include <QMenu>

namespace Ui {
class colorPickerDialog;
}

class ColorPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorPickerDialog(QWidget *parent = 0);
    ~ColorPickerDialog();
    void intializeActors(QVector<QString> actorsNames, QVector<QColor> colors);

private:
    void initializeDialogWindow();
    QFrame *  initializeActorFrame(QString actName, QVector<QColor> colors, int index);

    QVBoxLayout * vLayout;
    QVector<QColor> pickerColors;
    QVector<QPushButton *> colorButtons;
    QVector<QString> actorNamesList;

    QColor copyColorCode;

private slots:
    void showColorPickerWidget(bool bl);
    void donePushButtonClicked(bool bl);
    void showContextMenuForActorPB(QPoint pos);
signals:
    void changedColors(QVector<QColor> colors);
};

#endif // COLORPICKERDIALOG_H
