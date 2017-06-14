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

#ifndef XMLPARSER_H
#define XMLPARSER_H

#include <QFile>
#include <QFileDialog>
#include <QTableWidget>
#include <QXmlStreamReader>
#include <QStandardItemModel>
#include <QDir>

class Xmlparser  : public QObject
{
    Q_OBJECT
public:
    explicit Xmlparser(QString homeDir);

public slots: // convert to signal slots
    void openXmlFile(QString xmlFilePath);
    void readXmlFile();
    void saveToXmlFile(QStringList parameters, QStandardItemModel * smpData, QStandardItemModel *affMatrix, QString path);
    void saveNewDataToXmlFile(QStringList parameters, QTableWidget * smpDataWidget, QTableWidget *affintyDataWidget);
    void updateHomeDir(QString dir);

private :
    void processModelParameters();
    void processDimensions();
    void ProcessActors();
    void ProcessIdealAdjustments();
    void organizeXmlData();

    QString xmlFileName;
    QXmlStreamReader xmlReader;

    QString name;
    QString desc;
    QString prngSeed;
    QStringList modelParametes;
    QStringList dimensionNames;
    QList <QStringList> actorsList;
    QVector <QStringList> idealAdjustmentList;

    QStandardItemModel *xmlModel;
    QString homeDirectory;


    void tagElements(QXmlStreamWriter *xmlWriter, QString tagName, QString tagValue);
signals:
    void openXMLStatus(bool bl);
    void xmlParsedData(QStringList modelDesc, QStringList modpara, QStringList dims,
                       QStandardItemModel * actModel, QVector <QStringList> idealAdj);
    void newXmlFilePath(QString path);

};

#endif // XMLPARSER_H
