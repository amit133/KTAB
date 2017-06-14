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

#include "mainwindow.h"

void MainWindow::createXmlTable()
{
    tableType="XML";
    turnSlider->hide();
    tableControlsFrame->show();
    if (stackWidget->count()>1)
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(csvTableViewTabWidget);
    }

    for (int i =0; i< xmlTabWidget->count(); ++i)
    {
        xmlTabWidget->removeTab(0);//count 2, remove 1st
        xmlTabWidget->removeTab(0);//count 1, remove 1st
    }

    xmlImportDataTableView = new QTableView;
    xmlAffinityMatrixTableView = new QTableView;
    xmlAffinityMatrixTableView->setToolTip("The affinity matrix records, for all pairwise comparisons of actors, "
                                           "\nthe affinity which actor i has for the position of actor j");
    xmlTabWidget->addTab(xmlImportDataTableView,"Xml SMP Data");
    xmlTabWidget->addTab(xmlAffinityMatrixTableView,"Affinity Matrix");
    xmlTabWidget->setTabToolTip(1,"The affinity matrix records, for all pairwise comparisons of actors, "
                                  "\nthe affinity which actor i has for the position of actor j");
    stackWidget->addWidget(xmlTabWidget);

}

void MainWindow::importXmlGetFilePath(bool bl,QString filepath)
{
    QString filename;
    if(filepath.isEmpty())
    {
        filename = QFileDialog::getOpenFileName(this,tr("Xml File"),homeDirectory, tr("Xml File (*.xml)"));
    }
    else
        filename=filepath;
    if(!filename.isEmpty())
    {
        QDir dir =QFileInfo(filename).absoluteDir();
        homeDirectory = dir.absolutePath();

        setCurrentFile(filename);

        lineGraphDock->setVisible(false);
        barGraphDock->setVisible(false);
        quadMapDock->setVisible(false);

        xmlPath=filename;
        emit openXMLFile(filename);
    }
    stackWidget->setCurrentIndex(1);
}

void MainWindow::openStatusXml(bool status)
{
    if(status)
    {
        savedAsXml=true;
        createXmlTable();
        emit readXMLFile();

        donePushButton->setEnabled(true);
        donePushButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");
        runButton->setEnabled(true);
        dimensionsLineEdit->setEnabled(true);
        addActorsPushButton->setEnabled(true);
        deleteActorsPushButton->setEnabled(true);
        dimensionsPushButton->setEnabled(true);

        clearAllGraphs();

    }
    else
        displayMessage("Xml Parser", "Unable to open File");

}

void MainWindow::xmlDataParsedFromFile(QStringList modelDesc, QStringList modpara,
                                       QStringList dims, QStandardItemModel * actModel,
                                       QVector<QStringList> idealAdj)
{
    dimensionsLineEdit->setText(QString::number(dims.length()));
    dimensionsLineEdit->setEnabled(true);
    runButton->setEnabled(true);
    runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

    dimensionsXml=dims;

    updateControlsBar(modelDesc);
    updateModelParameters(modpara);
    populateXmlTable(actModel);

    QVector<QString> actors;
    for(int i =0 ; i < actModel->rowCount() ; ++ i)
        actors.append(actModel->item(i,0)->text());

    populateAffinityMatrix(idealAdj,actors);
    //    qDebug()<< modelDesc.length() << modpara.length() << dims.length() << actModel->rowCount() << idealAdj.length();
}

void MainWindow::savedXmlName(QString fileName)
{
    xmlPath = fileName;
}

void MainWindow::populateXmlTable(QStandardItemModel *actorsVal)
{
    tableType="XML";

    plotQuadMap->setEnabled(false);
    removeAllScatterPoints();

    turnSlider->hide();
    xmlSmpDataModel = new QStandardItemModel;

    QStringList xmlTableHeaders;
    xmlTableHeaders<< "Actor" << "Description" << "Power"
                   << "Position " << "Salience ";

    xmlSmpDataModel = actorsVal;
    for(int col =0 ; col <3;++col)
    {
        xmlSmpDataModel->setHeaderData(col,Qt::Horizontal,xmlTableHeaders.at(col));
    }
    xmlSmpDataModel->horizontalHeaderItem(0)->setToolTip("An individual, institution or group");
    xmlSmpDataModel->horizontalHeaderItem(1)->setToolTip("A description of the actor");
    xmlSmpDataModel->horizontalHeaderItem(2)->setToolTip("How much can the actor influence other actors");

    int dim=0;
    for(int col = 3 ; col <xmlSmpDataModel->columnCount(); col+=2)
    {
        QString pos(" \n Position ");
        QString sal(" \n Salience ");

        pos.prepend(dimensionsXml.at(dim));
        sal.prepend(dimensionsXml.at(dim));

        xmlSmpDataModel->setHeaderData(col,Qt::Horizontal,pos);
        xmlSmpDataModel->horizontalHeaderItem(col)->
                setToolTip("The policy position of an actor regarding the question ( with or against)");
        xmlSmpDataModel->setHeaderData(col+1,Qt::Horizontal,sal);
        xmlSmpDataModel->horizontalHeaderItem(col+1)->
                setToolTip("How much the actor cares about the question");

        ++dim;
    }

    actorsLineEdit->setText(QString::number(xmlSmpDataModel->rowCount()));
    actorsLineEdit->setEnabled(true);
    scenarioNameLineEdit->setPlaceholderText("Dataset/Scenario name");
    scenarioDescriptionLineEdit->setPlaceholderText("Dataset/Scenario Description");
    xmlImportDataTableView->setModel(xmlSmpDataModel);
    xmlImportDataTableView->showMaximized();
    xmlImportDataTableView->resizeColumnsToContents();
    xmlImportDataTableView->resizeColumnToContents(1);
    xmlImportDataTableView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    xmlImportDataTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    xmlImportDataTableView->setShowGrid(true);

    connect(xmlImportDataTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuXmlTableView(QPoint)));
    connect(xmlSmpDataModel,SIGNAL(itemChanged(QStandardItem*)),this, SLOT(xmlCellSelected(QStandardItem*)));
}

void MainWindow::populateAffinityMatrix(QVector<QStringList> idealAdj, QVector<QString> actors)
{
    QStandardItemModel * affinityModel = new QStandardItemModel;

    xmlAffinityMatrixModel = new QStandardItemModel;
    for(int act = 0 ; act < actors.length() ; ++act)
    {
        affinityModel->setHorizontalHeaderItem(act,new QStandardItem(actors.at(act)));
        affinityModel->setVerticalHeaderItem(act,new QStandardItem(actors.at(act)));

        for(int actH =0 ; actH <actors.length(); ++actH)
        {
            if(act==actH)
                affinityModel->setItem(act,actH, new QStandardItem("1"));
            else
                affinityModel->setItem(act,actH, new QStandardItem("0"));
        }
    }

    for(int i=0; i< idealAdj.length();++i)
    {
        affinityModel->setItem(actors.indexOf(idealAdj.at(i).at(1)),
                               actors.indexOf(idealAdj.at(i).at(0)),new QStandardItem(idealAdj.at(i).at(2)));
    }

    xmlAffinityMatrixModel=affinityModel;
    xmlAffinityMatrixTableView->setModel(xmlAffinityMatrixModel);
    xmlAffinityMatrixTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    xmlAffinityMatrixTableView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    connect(xmlSmpDataModel,SIGNAL(itemChanged(QStandardItem*)),this, SLOT(xmlCellSelected(QStandardItem*)));
    //    connect(xmlAffinityMatrixTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuXmlAffTableView(QPoint)));

}

void MainWindow::updateControlsBar(QStringList modelDesc)
{
    scenarioComboBox->clear();
    scenarioComboBox->setVisible(false);
    scenarioNameLineEdit->setVisible(true);
    scenarioNameLineEdit->setText(modelDesc.at(0));

    scenarioDescriptionLineEdit->setText(modelDesc.at(1));//desc
    scenarioDescriptionLineEdit->setEnabled(true);

    seedRand->setText(modelDesc.at(2)); // prng seed
}

void MainWindow::updateModelParameters(QStringList modPara)
{
    if(modPara.length()==9)
    {
        victProbModelComboBox->setCurrentIndex(victProbModelComboBox->findText(modPara.at(0)));
        pCEModelComboBox->setCurrentIndex(pCEModelComboBox->findText(modPara.at(1)));
        stateTransitionsComboBox->setCurrentIndex(stateTransitionsComboBox->findText(modPara.at(2)));
        votingRuleComboBox->setCurrentIndex(votingRuleComboBox->findText(modPara.at(3)));
        bigRAdjustComboBox->setCurrentIndex(bigRAdjustComboBox->findText(modPara.at(4)));
        bigRRangeComboBox->setCurrentIndex(bigRRangeComboBox->findText(modPara.at(5)));
        thirdPartyCommitComboBox->setCurrentIndex(thirdPartyCommitComboBox->findText(modPara.at(6)));
        interVecBrgnComboBox->setCurrentIndex(interVecBrgnComboBox->findText(modPara.at(7)));
        bargnModelComboBox->setCurrentIndex(bargnModelComboBox->findText(modPara.at(8)));
    }
    else
    {
        victProbModelComboBox->setCurrentIndex(defParameters.at(0));
        pCEModelComboBox->setCurrentIndex(defParameters.at(1));
        stateTransitionsComboBox->setCurrentIndex(defParameters.at(2));
        votingRuleComboBox->setCurrentIndex(defParameters.at(3));
        bigRAdjustComboBox->setCurrentIndex(defParameters.at(4));
        bigRRangeComboBox->setCurrentIndex(defParameters.at(5));
        thirdPartyCommitComboBox->setCurrentIndex(defParameters.at(6));
        interVecBrgnComboBox->setCurrentIndex(defParameters.at(7));
        bargnModelComboBox->setCurrentIndex(defParameters.at(8));

        displayMessage("XML", "No/Insufficient Model Parameters");
    }
}

void MainWindow::initializeAffinityMatrixRowCol(int count, QString table)
{
    if("NewSMPData"==table || "DatabaseEdit"== table)
    {
        for( int col = 0; col < affinityMatrix->columnCount(); ++col)
        {
            if(count==col)
                affinityMatrix->setItem(count,col,new QTableWidgetItem("1"));
            else
                affinityMatrix->setItem(count,col,new QTableWidgetItem("0"));
        }

        for(int row =0; row < affinityMatrix->rowCount() ; ++row)
        {
            if(count==row)
                affinityMatrix->setItem(row,count,new QTableWidgetItem("1"));
            else
                affinityMatrix->setItem(row,count,new QTableWidgetItem("0"));
        }

    }
    else if ("XML"==table)
    {
        for( int col = 0; col < xmlAffinityMatrixModel->columnCount(); ++col)
        {
            if(count==col)
                xmlAffinityMatrixModel->setItem(count,col,new QStandardItem("1"));
            else
                xmlAffinityMatrixModel->setItem(count,col,new QStandardItem("0"));
        }

        for(int row =0; row < xmlAffinityMatrixModel->rowCount() ; ++row)
        {
            if(count==row)
                xmlAffinityMatrixModel->setItem(row,count,new QStandardItem("1"));
            else
                xmlAffinityMatrixModel->setItem(row,count,new QStandardItem("0"));
        }
    }
    else if ("CSV"==table)
    {
        for( int col = 0; col < csvAffinityModel->columnCount(); ++col)
        {
            if(count==col)
                csvAffinityModel->setItem(count,col,new QStandardItem("1"));
            else
                csvAffinityModel->setItem(count,col,new QStandardItem("0"));
        }

        for(int row =0; row < csvAffinityModel->rowCount() ; ++row)
        {
            if(count==row)
                csvAffinityModel->setItem(row,count,new QStandardItem("1"));
            else
                csvAffinityModel->setItem(row,count,new QStandardItem("0"));
        }
    }
}

void MainWindow::saveTableViewToXML()
{
    savedAsXml=true;
    if(0==validateControlButtons("xml_tableView"))
    {
        QStringList parameters;
        parameters.append(scenarioNameLineEdit->text());
        parameters.append(scenarioDescriptionLineEdit->text());
        if(!seedRand->text().isEmpty())
            parameters.append(seedRand->text());
        else
            parameters.append("0");
        parameters.append(victProbModelComboBox->currentText());
        parameters.append(pCEModelComboBox->currentText());
        parameters.append(stateTransitionsComboBox->currentText());
        parameters.append(votingRuleComboBox->currentText());
        parameters.append(bigRAdjustComboBox->currentText());
        parameters.append(bigRRangeComboBox->currentText());
        parameters.append(thirdPartyCommitComboBox->currentText());
        parameters.append(interVecBrgnComboBox->currentText());
        parameters.append(bargnModelComboBox->currentText());

        QStringList dims;
        for(int i=3; i <xmlSmpDataModel->columnCount();++i)
        {
            dims.append(xmlSmpDataModel->headerData(i,Qt::Horizontal).toString().remove("Position").remove("\n"));
            ++i;
        }
        parameters.append(dims);

        QString filename = QFileDialog::getSaveFileName(this,tr("Save Xml"), homeDirectory ,tr("Xml files (*.xml)"));

        if(!filename.endsWith(".xml"))
            filename.append(".xml");

        if(!filename.isEmpty())
        {
            QDir dir =QFileInfo(filename).absoluteDir();
            homeDirectory = dir.absolutePath();
            setCurrentFile(filename);
        }

        emit saveXMLDataToFile(parameters,xmlSmpDataModel,xmlAffinityMatrixModel,filename);

        runButton->setEnabled(true);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

    }
}

//void MainWindow::displayAffinityMenuTableWidget(QPoint pos)
//{
//    QMenu menu(this);
//    QAction *newActor = menu.addAction("Insert New Actor w.r.t row");
//    QAction *delActor = menu.addAction("Delete Actor w.r.t row");
//    menu.addSeparator();
//    QAction *rename = menu.addAction("Rename Header w.r.t row");
//    QAction *act = menu.exec(affinityMatrix->viewport()->mapToGlobal(pos));

//    if (act == newActor)
//    {
//        bool ok;
//        QString text = QInputDialog::getText(this, tr("Plesase Enter the Actor Name"),
//                                             tr("Header Name"), QLineEdit::Normal," Actor ", &ok);
//        if (ok && !text.isEmpty())
//        {
//            affinityMatrix->insertColumn(affinityMatrix->currentRow());
//            affinityMatrix->insertRow(affinityMatrix->currentRow());
//            affinityMatrix->setHorizontalHeaderItem(affinityMatrix->currentRow()-1,new QTableWidgetItem(text));
//            affinityMatrix->setVerticalHeaderItem(affinityMatrix->currentRow()-1,new QTableWidgetItem(text));
//            statusBar()->showMessage("Header updated, Actor Inserted");

//            for( int col = 0; col < affinityMatrix->columnCount(); ++col)
//            {
//                if(affinityMatrix->currentRow()-1==col)
//                    affinityMatrix->setItem(affinityMatrix->currentRow()-1,col,new QTableWidgetItem("1"));
//                else
//                    affinityMatrix->setItem(affinityMatrix->currentRow()-1,col,new QTableWidgetItem("0"));
//            }

//            for(int row = 0; row < affinityMatrix->rowCount() ; ++row)
//            {
//                if(affinityMatrix->currentRow()-1==row)
//                    affinityMatrix->setItem(row,affinityMatrix->currentRow()-1,new QTableWidgetItem("1"));
//                else
//                    affinityMatrix->setItem(row,affinityMatrix->currentRow()-1,new QTableWidgetItem("0"));
//            }
//        }
//    }
//    if (act == rename)
//    {
//        bool ok;
//        QString text = QInputDialog::getText(this, tr("Plesase Enter the Actor Name"),
//                                             tr("Header Name"), QLineEdit::Normal,
//                                             affinityMatrix->horizontalHeaderItem(affinityMatrix->currentRow())->text(), &ok);

//        if (ok && !text.isEmpty())
//        {
//            affinityMatrix->setHorizontalHeaderItem(affinityMatrix->currentRow(),new QTableWidgetItem(text));
//            affinityMatrix->setVerticalHeaderItem(affinityMatrix->currentRow(),new QTableWidgetItem(text));
//            statusBar()->showMessage("Header updated");
//        }
//    }
//    if (act == delActor)
//    {
//        affinityMatrix->removeColumn(affinityMatrix->currentRow());
//        affinityMatrix->removeRow(affinityMatrix->currentRow());
//        statusBar()->showMessage("Actor Removed");
//    }
//}

void MainWindow::displayMenuXmlTableView(QPoint pos)
{
    QMenu menu(this);
    QAction *posCol = menu.addAction("Insert Position Column");
    QAction *salCol = menu.addAction("Insert Salience Column");
    menu.addSeparator();
    QAction *newRow = menu.addAction("Insert Row");
    menu.addSeparator();
    QAction *col = menu.addAction("Remove Column");
    QAction *row = menu.addAction("Remove Row");
    menu.addSeparator();
    QAction *rename = menu.addAction("Rename Column Header");

    QAction *act = menu.exec(xmlImportDataTableView->viewport()->mapToGlobal(pos));

    if (act == col)
    {
        if(xmlImportDataTableView->currentIndex().column()>2)
            xmlSmpDataModel->removeColumn(xmlImportDataTableView->currentIndex().column());
        else
            statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
    }
    if (act == row)
    {
        xmlAffinityMatrixModel->removeRow(xmlImportDataTableView->currentIndex().row());
        xmlAffinityMatrixModel->removeColumn(xmlImportDataTableView->currentIndex().row());
        xmlSmpDataModel->removeRow(xmlImportDataTableView->currentIndex().row());
    }
    if (act == rename)
    {
        if(xmlImportDataTableView->currentIndex().column()>2)
        {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                 tr("Header Name"), QLineEdit::Normal,
                                                 xmlSmpDataModel->headerData(xmlImportDataTableView->currentIndex().column(),
                                                                             Qt::Horizontal).toString(), &ok);
            if (ok && !text.isEmpty())
            {
                if(xmlImportDataTableView->currentIndex().column()%2!=0)
                {
                    if(!(text.contains("Position") || text.contains("position")))
                    {
                        if(!text.contains("\n"))
                            text = text.append("\n Position");
                        else
                            text = text.append(" Position");
                    }
                    else
                    {   QString header = text;
                        header.remove("Position").remove("position");
                        header.remove("\n");
                        header.append("\n Position");
                        text = header;
                    }
                    xmlSmpDataModel->horizontalHeaderItem(xmlImportDataTableView->currentIndex().column())->
                            setToolTip("The policy position of an actor regarding the question ( with or against)");
                }
                else
                {
                    if(!(text.contains("Salience")|| text.contains("salience")))
                    {
                        if(!text.contains("\n"))
                            text = text.append("\n Salience");
                        else
                            text = text.append(" Salience");
                    }
                    else
                    {
                        QString header = text;
                        header.remove("Salience").remove("salience");
                        header.remove("\n");
                        header.append("\n Salience");
                        text = header;
                    }

                    xmlSmpDataModel->horizontalHeaderItem(xmlImportDataTableView->currentIndex().column())->
                            setToolTip("How much the actor cares about the question");
                }
                xmlSmpDataModel->setHeaderData(xmlImportDataTableView->currentIndex().column(),Qt::Horizontal,text);
                statusBar()->showMessage("Header changed");
            }
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }
    if (act == posCol)
    {
        if(xmlImportDataTableView->currentIndex().column()>2)
        {
            xmlSmpDataModel->insertColumn(xmlImportDataTableView->currentIndex().column());
            xmlSmpDataModel->setHeaderData(xmlImportDataTableView->currentIndex().column()-1,Qt::Horizontal,"Position");
            xmlSmpDataModel->horizontalHeaderItem(xmlImportDataTableView->currentIndex().column()-1)->
                    setToolTip("The policy position of an actor regarding the question ( with or against)");
            statusBar()->showMessage("Column Inserted, Header changed");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }
    if (act == salCol)
    {
        if(xmlImportDataTableView->currentIndex().column()>2)
        {
            xmlSmpDataModel->insertColumn(xmlImportDataTableView->currentIndex().column());
            xmlSmpDataModel->setHeaderData(xmlImportDataTableView->currentIndex().column()-1,Qt::Horizontal,"Salience");
            xmlSmpDataModel->horizontalHeaderItem(xmlImportDataTableView->currentIndex().column()-1)->setToolTip("How much the actor cares about the question");
            statusBar()->showMessage("Column Inserted, Header changed");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if(act == newRow)
    {
        xmlAffinityMatrixModel->insertColumn(xmlImportDataTableView->currentIndex().row());
        xmlAffinityMatrixModel->insertRow(xmlImportDataTableView->currentIndex().row());

        QString actorHeader;
        actorHeader.append(" Actor ")/*.append(QString::number(rows+1))*/;
        xmlAffinityMatrixModel->setHorizontalHeaderItem(xmlImportDataTableView->currentIndex().row(),
                                                        new QStandardItem(actorHeader));
        xmlAffinityMatrixModel->setVerticalHeaderItem(xmlImportDataTableView->currentIndex().row(),
                                                      new QStandardItem(actorHeader));
        initializeAffinityMatrixRowCol(xmlImportDataTableView->currentIndex().row(),"XML");
        actorHeader.clear();
        xmlSmpDataModel->insertRow(xmlImportDataTableView->currentIndex().row());
    }
}

//void MainWindow::displayMenuXmlAffTableView(QPoint pos)
//{
//    QMenu menu(this);
//    QAction *newactor = menu.addAction("Insert New Actor w.r.t row");
//    QAction *delactor = menu.addAction("Delete Actor w.r.t row");
//    menu.addSeparator();
//    QAction *rename = menu.addAction("Rename Header w.r.t row");

//    QAction *act = menu.exec(xmlAffinityMatrixTableView->viewport()->mapToGlobal(pos));

//    if (act == newactor)
//    {
//        bool ok;
//        QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
//                                             tr("Header Name"), QLineEdit::Normal,"Actor ", &ok);
//        xmlAffinityMatrixModel->insertColumn(xmlAffinityMatrixTableView->currentIndex().row());
//        xmlAffinityMatrixModel->insertRow(xmlAffinityMatrixTableView->currentIndex().row());

//        if(ok && !text.isEmpty())
//        {
//            xmlAffinityMatrixModel->setVerticalHeaderItem(xmlAffinityMatrixTableView->currentIndex().row()-1,
//                                                          new QStandardItem(text));
//            xmlAffinityMatrixModel->setHorizontalHeaderItem(xmlAffinityMatrixTableView->currentIndex().row()-1,
//                                                            new QStandardItem(text));
//        }
//        for( int col = 0; col < xmlAffinityMatrixModel->columnCount(); ++col)
//        {
//            if(xmlAffinityMatrixTableView->currentIndex().row()-1==col)
//                xmlAffinityMatrixModel->setItem(xmlAffinityMatrixTableView->currentIndex().row()-1,col,
//                                                new QStandardItem("1"));
//            else
//                xmlAffinityMatrixModel->setItem(xmlAffinityMatrixTableView->currentIndex().row()-1,col,
//                                                new QStandardItem("0"));
//        }

//        for(int row = 0; row < xmlAffinityMatrixModel->rowCount() ; ++row)
//        {
//            if(xmlAffinityMatrixTableView->currentIndex().row()-1==row)
//                xmlAffinityMatrixModel->setItem(row,xmlAffinityMatrixTableView->currentIndex().row()-1,
//                                                new QStandardItem("1"));
//            else
//                xmlAffinityMatrixModel->setItem(row,xmlAffinityMatrixTableView->currentIndex().row()-1,
//                                                new QStandardItem("0"));
//        }
//    }
//    if (act == delactor)
//    {
//        xmlAffinityMatrixModel->removeColumn(xmlAffinityMatrixTableView->currentIndex().row());
//        xmlAffinityMatrixModel->removeRow(xmlAffinityMatrixTableView->currentIndex().row());
//    }
//    if (act == rename)
//    {
//        bool ok;
//        QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
//                                             tr("Header Name"), QLineEdit::Normal,xmlAffinityMatrixModel->headerData(
//                                                 xmlAffinityMatrixTableView->currentIndex().row(),Qt::Horizontal).toString(),
//                                             &ok);
//        if (ok && !text.isEmpty())
//        {
//            xmlAffinityMatrixModel->setVerticalHeaderItem(xmlAffinityMatrixTableView->currentIndex().row(),
//                                                          new QStandardItem(text));
//            xmlAffinityMatrixModel->setHorizontalHeaderItem(xmlAffinityMatrixTableView->currentIndex().row(),
//                                                            new QStandardItem(text));
//            statusBar()->showMessage("Header changed");
//        }
//    }
//}
