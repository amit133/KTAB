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

#ifndef CSV_H
#define CSV_H

#include <QStandardItemModel>
#include <QFile>
#include <QTextStream>
#include <QRegExp>

class CSV : public QObject
{
    Q_OBJECT
public:
    CSV();
    QStandardItemModel * model;
public slots:
    void readCSVFile(QString path);
    void exportActorColors(QString path, QVector<int> actorIds, QVector<QString> colorCode);
    void importActorColors(QString path,int actCount);

signals:
    void csvModel(QStandardItemModel * ,QStringList ScenarioName);
    void sendMessage(QString className, QString Message);
    void importedColors(QVector<QColor> colors);

};

#endif // CSV_H
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
