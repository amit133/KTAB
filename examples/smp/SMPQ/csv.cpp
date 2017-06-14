
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
// --------------------------------------------

#include "csv.h"
#include <QDebug>

CSV::CSV()
{
    model = new QStandardItemModel;
}
void CSV::readCSVFile(QString path)
{
    QRegExp space("^\\s*$");
    //getting the csv file path
    QStringList scenarioName;
    if(!path.isEmpty())
    {
        // model->clear();

        QFile file(path);
        if (file.open(QIODevice::ReadOnly) && file.size() > 0)
        {
            int rowindex = 0;                     // file row counter
            QTextStream in(&file);                 // read to text stream

            while (!in.atEnd())
            {
                // read one line from textstream(separated by "\n")
                QString fileLine = in.readLine();

                // parse the read line into separate pieces(tokens) with "," as the delimiter
                QStringList lineToken = fileLine.split(",", QString::SkipEmptyParts);
                QString lastField = lineToken.at(lineToken.length()-1);
                if(lastField.contains(space))
                    lineToken.removeLast();

                if(lineToken.length()>2)//CHECK FOR COLORS CSV
                {
                    if(rowindex >= 2 )
                    {
                        // load parsed data to model accordingly
                        for (int j = 0; j < lineToken.size(); j++)
                        {
                            if(j > 1)
                            {
                                QString value = lineToken.at(j);
                                value = QString::number(value.toFloat(),'f',2);
                                QStandardItem *item = new QStandardItem(value.trimmed());
                                model->setItem(rowindex-2, j, item);
                            }
                            else
                            {
                                QString value = lineToken.at(j);
                                QStandardItem *item = new QStandardItem(value.trimmed());
                                model->setItem(rowindex-2, j, item);
                            }
                        }
                    }
                    else if(rowindex == 1)
                    {
                        for(int i=3; i< lineToken.length(); ++i)
                        {
                            QString header = lineToken.at(i);
                            QStringList headerList = header.split(" ");
                            if(headerList.length()>1)
                            {
                                header = headerList.at(0);
                                header.append(" \n").append(headerList.at(1));
                                lineToken[i]=header;
                            }
                        }

                        model->setHorizontalHeaderLabels(lineToken);
                    }
                    else if(rowindex == 0)
                    {
                        scenarioName.append(lineToken.at(0));
                        scenarioName.append(lineToken.at(1));
                        lineToken.clear();
                    }
                    rowindex++;
                }
                else
                {
                    emit sendMessage("CSV","Unknown Data !");
                    return;
                }
            }
            file.close();
            emit csvModel(model,scenarioName);
        }
        else
        {
            emit sendMessage("CSV","Empty File !");
            return;
        }
    }
}


void CSV::exportActorColors(QString path, QVector<int> actorIds, QVector<QString> colorCode)
{
    QFile f(path);

    if (f.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream data( &f );
        QStringList strList;

        //appending data
        for (int row=0; row < actorIds.length(); row++)
        {
            strList.clear();

            strList << QString::number(actorIds.at(row));
            strList << colorCode.at(row);
            data << strList.join(",") + "\n";
//            qDebug() << strList;
        }
        f.close();
    }
}

void CSV::importActorColors(QString path, int actCount)
{
    QRegExp space("^\\s*$");

    //getting the csv file path
    QVector<QColor> colorCodeList;
    if(!path.isEmpty())
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly) && file.size() > 0)
        {
            // file row counter
            QTextStream in(&file);                 // read to text stream

            while (!in.atEnd())
            {
                // read one line from textstream(separated by "\n")
                QString fileLine = in.readLine();

                // parse the read line into separate pieces(tokens) with "," as the delimiter
                QStringList lineToken = fileLine.split(",", QString::SkipEmptyParts);
                QString lastField = lineToken.at(lineToken.length()-1);
                if(lastField.contains(space))
                    lineToken.removeLast();


                // load parsed data to model accordingly
                for (int j = 0; j < lineToken.size(); j++)
                {
                    if(j == 1)
                    {
                        colorCodeList.append(lineToken.at(j));
                    }
                }
            }
            file.close();
        }
//        qDebug()<<colorCodeList.length() << actCount;

        emit(importedColors(colorCodeList));


    }
    else
    {
        emit sendMessage("CSV","File is Empty ! ");
        return;
    }
}
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
