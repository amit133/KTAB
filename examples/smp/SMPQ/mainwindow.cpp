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
#include "ui_mainwindow.h"

MainWindow::MainWindow()
{
    initializeCentralViewFrame();

    createActions();
    createStatusBar();

    createModelParametersDockWindow();
    createLinePlotsDockWindows();
    createBarPlotsDockWindows();
    createQuadMapDockWindows();

    intializeHomeDirectory();

    setWindowTitle(tr("KTAB: Spatial Model of Politics (SMP)"));

    //CSV class obj
    csvObj = new CSV;
    //connect statement to pass the csv file path
    connect(this,SIGNAL(csvFilePath(QString)), csvObj, SLOT(readCSVFile(QString)));
    //connect statement to receive processed model
    connect(csvObj,SIGNAL(csvModel(QStandardItemModel*,QStringList)),
            this,SLOT(setCSVItemModel(QStandardItemModel*,QStringList)));
    //To display any message to user
    connect(csvObj,SIGNAL(sendMessage(QString,QString)),this,SLOT(displayMessage(QString,QString)));

    //Database
    dbObj = new Database();
    //To open database by passing the path
    connect(this,SIGNAL(dbFilePath(QString,QString,QString,bool)),dbObj, SLOT(openDB(QString,QString,QString,bool)));

    //To get Database item Model to show on GUI
    connect(dbObj,SIGNAL(dbModel(QStandardItemModel*)),this,SLOT(setDBItemModel(QStandardItemModel*)));
    //To get Database item Model to show on Edit
    //    connect(dbObj,&Database::dbModelEdit,this,&MainWindow::setDBItemModelEdit);
    //To display any message to user
    connect(dbObj,SIGNAL(Message(QString,QString)),this,SLOT(displayMessage(QString,QString)));
    //To get vector positions of actors to plot a graph on GUI
    connect(dbObj,SIGNAL(vectorPosition(QVector<double>,QVector<double>,QString,int))
            ,this,SLOT(addGraphOnLinePlot(QVector<double>,QVector<double>,QString,int)));
    //To get number of states to set the max values of slider
    connect(dbObj,SIGNAL(statesCount(int)),this,SLOT(updateStateCountSliderRange(int)));

    //To get number of actors
    connect(dbObj,SIGNAL(actorCount(int)),this,SLOT(updateActorCount(int)));

    //To get all scenarios from Database to update ScenarioComboBox
    connect(dbObj,SIGNAL(scenarios(QStringList*,QStringList*,QStringList*,int)),
            this,SLOT(updateScenarioListComboBox(QStringList*,QStringList*,QStringList*,int)));
    //To get initial scenario vector positions and Item model to update graph and Table view
    connect(this, SIGNAL(getScenarioRunValues(int,QString,int)),dbObj,SLOT(getScenarioData(int,QString,int)));
    //To store scenario value, for edit database to save as csv reference
    connect(this, SIGNAL(getScenarioRunValuesEdit(QString)),dbObj,SLOT(getScenarioDataEdit(QString)));
    //To get state count
    connect(this, SIGNAL(getStateCountfromDB()),dbObj,SLOT(getStateCount()));
    //To get dimensions count
    connect(this,SIGNAL(getDimensionCountfromDB()),dbObj,SLOT(getDimensionCount()));
    //received dimension Values
    connect(dbObj,SIGNAL(dimensionsCount(int, QStringList*)),this,SLOT(updateDimensionCount(int, QStringList*)));

    //DB to Tables
    connect(this, SIGNAL(getActorsDesc()),dbObj,SLOT(getActorsDescriptionDB()));
    connect(dbObj,SIGNAL(actorsNameDesc(QVector <QString> ,QVector <QString>)),this,SLOT(actorsNameDesc(QVector  <QString> ,QVector  <QString>)));
    connect(this, SIGNAL(getInfluence(int)),dbObj,SLOT(getInfluenceDB(int)));
    connect(dbObj,SIGNAL(actorsInflu(QVector<QString>)),this,SLOT(actorsInfluence(QVector  <QString>)));
    connect(this, SIGNAL(getPosition(int,int)),dbObj,SLOT(getPositionDB(int,int)));
    connect(dbObj,SIGNAL(actorsPostn(QVector<QString>,int)),this,SLOT(actorsPosition(QVector<QString>,int)));
    connect(this, SIGNAL(getSalience(int,int)),dbObj,SLOT(getSalienceDB(int,int)));
    connect(dbObj,SIGNAL(actorsSalnce(QVector<QString>,int)),this,SLOT(actorsSalience(QVector<QString>,int)));
    connect(dbObj,SIGNAL(actorsAffinity(QVector<QString>,QVector<int>,QVector<int>)),
            this,SLOT(actAffinity(QVector<QString>,QVector<int>,QVector<int>)));
    connect(dbObj,SIGNAL(scenModelParameters(QVector<int>,QString)),
            this,SLOT(scenarioModelParameters(QVector<int>,QString)));
    connect(this,SIGNAL(releaseDatabase()),dbObj,SLOT(releaseDB()));
    //Postgres DB
    connect(this,SIGNAL(getPostgresDBList(QString,bool)),dbObj, SLOT(checkPostgresDB(QString,bool)));
    //dblist
    connect(dbObj,SIGNAL(postgresExistingDBList(QStringList*,bool)),this,SLOT(postgresDBList(QStringList*,bool)));


    //BAR Charts
    connect(this,SIGNAL(getActorIdsInRange(double,double,int,int)),dbObj,SLOT(getActorsInRangeFromDB(double,double,int,int)));
    connect(dbObj,SIGNAL(listActorsSalienceCapability(QVector<int>,QVector<double>,QVector<double>,double,double)),this,
            SLOT(barGraphActorsSalienceCapability(QVector<int>,QVector<double>,QVector<double>,double,double)));

    //LINE GRAPHS
    //To get dimensions count
    connect(this,SIGNAL(getDimforBar()),dbObj,SLOT(getDims()));
    //received dimension Values
    connect(dbObj,SIGNAL(dimensList(QStringList*)),this,SLOT(updateBarDimension(QStringList*)));
    connect(dbObj,SIGNAL(dimensList(QStringList*)),this,SLOT(updateLineDimension(QStringList*)));

    //QUAD MAPS
    //to get Util_Chlg and Util_SQ values
    connect(this,SIGNAL(getUtilChlgAndUtilSQfromDB(QVector<int>)),
            dbObj,SLOT(getUtilChlgAndSQvalues(QVector<int>)));

    //received Chlg and sq values
    connect(dbObj,SIGNAL(utilChlngAndSQ(int , double , double  , int )),
            this,SLOT(quadMapUtilChlgandSQValues(int , double , double  , int )));


    //XML Class signal slots
    xmlparser = new Xmlparser(homeDirectory);
    connect(this,SIGNAL(openXMLFile(QString)),xmlparser,SLOT(openXmlFile(QString)));
    connect(this,SIGNAL(readXMLFile()),xmlparser,SLOT(readXmlFile()));
    connect(xmlparser,SIGNAL(openXMLStatus(bool)),this,SLOT(openStatusXml(bool)));
    connect(xmlparser,SIGNAL(xmlParsedData(QStringList,QStringList,QStringList,QStandardItemModel*,
                                           QVector<QStringList>)),this,
            SLOT(xmlDataParsedFromFile(QStringList,QStringList,QStringList,QStandardItemModel*,
                                       QVector<QStringList>)));
    connect(this,SIGNAL(saveXMLDataToFile(QStringList,QStandardItemModel*,QStandardItemModel*,QString)),
            xmlparser,SLOT(saveToXmlFile(QStringList,QStandardItemModel*,QStandardItemModel*,QString)));
    connect(xmlparser,SIGNAL(newXmlFilePath(QString)),this,SLOT(savedXmlName(QString)));
    connect(this,SIGNAL(saveNewSMPDataToXMLFile(QStringList,QTableWidget*,QTableWidget*)),
            xmlparser,SLOT(saveNewDataToXmlFile(QStringList,QTableWidget*,QTableWidget*)));
    connect(this,SIGNAL(homeDirChanged(QString)),xmlparser,SLOT(updateHomeDir(QString)));

    //colorpalette
    connect(this,SIGNAL(exportColors(QString,QVector<int>, QVector<QString>)),
            csvObj,SLOT(exportActorColors(QString,QVector<int>,QVector<QString>)));
    connect(this ,SIGNAL(importColors(QString,int)),csvObj,SLOT(importActorColors(QString,int)));
    connect(csvObj,SIGNAL(importedColors(QVector<QColor>)),this,SLOT(updateColors(QVector<QColor>)));

    //actormoveddata
    connect(this,SIGNAL(getActorMovedData(QString)),dbObj,SLOT(getActorMovedDataDB(QString)));
    connect(dbObj,SIGNAL(actorMovedInfo(QStandardItemModel*)),this,SLOT(actorMovedInfoModel(QStandardItemModel*)));

    //editable headers of TableWidget and TableView
    headerEditor = 0;
    barsCount=0;
    yAxisLen=50;
    prevScenario="None";
    initiatorTip=0;
    prevTurn=0;
    firstVal=false;
    useHistory =true;
    currentScenarioId = "dummy";
    sankeyOutputHistory=true;
    menuconfig=false;
    importedDBFile = false;
    recentFileAccess.clear();
    connectionString.append("Driver=QSQLITE;");//connectionType
    connectionString.append("Server=localhost;");//host address
    connectionString.append("Database=").append("None").append(";");}

MainWindow::~MainWindow()
{
    emit releaseDatabase();
}

void MainWindow::csvGetFilePAth(bool bl, QString filepath )
{
    Q_UNUSED(bl)

    statusBar()->showMessage(tr("Looking for CSV file"));

    QString csvFilePth;
    if(filepath.isEmpty())
    {
        //Get  *.csv file path
        csvFilePth = QFileDialog::getOpenFileName(this,tr("Open CSV File"), homeDirectory , tr("CSV File (*.csv)"));
        if(!csvFilePth.isEmpty())
        {
            QDir dir =QFileInfo(csvFilePth).absoluteDir();
            homeDirectory = dir.absolutePath();
        }
    }
    else
        csvFilePth =filepath;

    //emit path to csv class for processing
    if(!csvFilePth.isEmpty())
    {
        setCurrentFile(csvFilePth);

        modeltoCSV->clear();
        emit csvFilePath(csvFilePth);
        csvPath = csvFilePth;
    }
    statusBar()->showMessage(tr(" "));
}

void MainWindow::dbGetFilePAth(bool bl, QString smpDBPath, bool run)
{

    Q_UNUSED(bl)
    statusBar()->showMessage(tr("Looking for Database file ..."));

    QString conType;
    QString currentPath = dbPath;
    if(!connectionString.isEmpty())
    {
        if(connectionString.contains("QSQLITE"))
        {
            conType="QSQLITE";
            if(smpDBPath.isEmpty())
            {
                //Get  *.db file path
                dbPath = QFileDialog::getOpenFileName(this,tr("Database File"), homeDirectory , tr("Database File (*.db)"));
                if(!dbPath.isEmpty())
                {
                    //    emit releaseDatabase();
                    QDir dir =QFileInfo(dbPath).absoluteDir();
                    homeDirectory = dir.absolutePath();
                    setCurrentFile(dbPath);
                }
            }
            else
            {
                dbPath = smpDBPath;
                if(dbPath.contains(".db"))
                {
                    setCurrentFile(dbPath);
                }
                else if(!dbPath.isEmpty())
                {
                    setCurrentFile(dbPath.append(".db"));
                    dbPath.remove(".db");
                }
            }
        }
        else if (connectionString.contains("QPSQL"))
        {
            conType="QPSQL";
            dbPath = smpDBPath;
            if(!dbPath.isEmpty())
            {
                setCurrentFile(dbPath);
            }
        }
        //emit path to db class for processing
        if(!dbPath.isEmpty())
        {
            clearAllLabels();
            lineGraphDock->setVisible(true);
            barGraphDock->setVisible(true);

            lineGraphDock->setEnabled(true);
            barGraphDock->setEnabled(true);

            disconnect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));
            disconnect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));
            mScenarioDesc.clear();
            mScenarioName.clear();
            mScenarioIds.clear();
            scenarioComboBox->clear();
            scenarioNameLineEdit->clear();
            connect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));
            connect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));

            modeltoDB->clear();
            if(run)
            {
                emit dbFilePath(dbPath,conType,connectionString,run);
            }
            else
            {
                emit dbFilePath(dbPath,conType,connectionString,false);
            }

            reconnectPlotWidgetSignals();
            //To populate Line Graph Dimensions combo box
            populateLineGraphDimensions(dimensionsLineEdit->text().toInt());
            //To populate Bar Graph Dimensions combo box
            populateBarGraphDimensions(dimensionsLineEdit->text().toInt());

        }
        else
        {
            dbPath = currentPath;
        }
        statusBar()->showMessage(tr(" "));
    }
    else
    {
        displayMessage("Database Import", "Database connection is not configured");
    }
}

void MainWindow::dbEditGetFilePAth(bool bl)
{
    Q_UNUSED(bl)
    if(dbPath.isEmpty())
    {
        displayMessage("Database File", "Import a Database first, then Click on \n - Edit Database to Save as CSV");
    }
    else
    {
        lineGraphDock->setVisible(false);
        barGraphDock->setVisible(false);
        quadMapDock->setVisible(false);

        emit getActorsDesc();
        emit getInfluence(0);

        for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
        {
            emit getSalience(i,0);
            emit getPosition(i,0);
        }
        setDBItemModelEdit();
    }
    clearAllGraphs();
}

void MainWindow::updateStateCountSliderRange(int states)
{
    turnSlider->setRange(0,states);

    //To set slider Range for line Graph
    populateLineGraphStateRange(states);
    //To set slider Range for Bar Graph
    populateBarGraphStateRange(states);
    //To set slider Range for Quad Map
    populateQuadMapStateRange(states);
}

void MainWindow::updateScenarioListComboBox(QStringList * scenarios,QStringList* scenarioIds,QStringList* scenarioDesc, int indx)
{
    for(int index=0;index<scenarios->length();++index)
    {
        mScenarioDesc.append(scenarioDesc->at(index));
        mScenarioIds.append(scenarioIds->at(index));
        mScenarioName.append(scenarios->at(index));

        disconnect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));
        scenarioComboBox->addItem(scenarios->at(index));
        connect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));

    }
    scenarioComboBox->setCurrentIndex(indx);
    scenarioBox = mScenarioIds.at(indx);
    scenarioDescriptionLineEdit->setText(mScenarioDesc.at(indx));

    QApplication::setOverrideCursor(QCursor(QPixmap("://images/hourglass.png")));
    statusBar()->showMessage("Please wait !! Data is being fetched from Database, "
                             "this may take some time !!");
    emit getActorMovedData(scenarioBox);
    QApplication::restoreOverrideCursor();


    //qDebug() <<scenarioBox;
    //    sliderStateValueToQryDB(0);//when new database is opened, start from zero
}

void MainWindow::updateDimensionCount(int dim, QStringList * dimList)
{
    dimensionList.clear();
    dimensions = dim;

    for(int i = 0 ; i < dimList->length(); ++i)
        dimensionList.append( dimList->at(i));
}

void MainWindow::updateActorCount(int actNum)
{
    numAct=actNum+1;
}

void MainWindow::sliderStateValueToQryDB(int value)
{
    turnSlider->setToolTip(QString("Current Turn: "+QString::number(value)));
    turnSlider->setToolTipDuration(3000);
    if(mScenarioIds.length()>0)
    {
        scenarioBox = mScenarioIds.at(scenarioComboBox->currentIndex());
        lineCustomGraph->clearGraphs();

        if(turnSlider->value()!=numStates)
            lineCustomGraph->xAxis->setRange(-1,turnSlider->value()+1);
        else
            lineCustomGraph->xAxis->setRange(-1,turnSlider->value());

        emit getScenarioRunValues(value,scenarioBox,dimension);

        lineGraphTitle->setText(QString(lineGraphDimensionComboBox->currentText()
                                        + ", Turn " +
                                        QString::number(value)));
        lineCustomGraph->yAxis->setLabel(lineGraphDimensionComboBox->currentText());
        lineCustomGraph->replot();

        //csv- DB
        emit getInfluence(value);
        for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
        {
            emit getSalience(i,value);
            emit getPosition(i,value);
        }

        updateDBViewColumns();
    }
}

void MainWindow::scenarioComboBoxValue(int scenario)
{
    removeAllGraphs();
    clearAllLabels();

    if(scenario>=0 && mScenarioIds.length()>0)
    {
        scenarioBox = mScenarioIds.at(scenario);
        if(currentScenarioId==scenarioBox)
        {
            useHistory=true;
        }
        else
        {
            useHistory=false;
        }
        scenarioDescriptionLineEdit->setText(mScenarioDesc.at(scenario));
        if(tableType=="Database")
        {
            QApplication::setOverrideCursor(QCursor(QPixmap("://images/hourglass.png")));
            QApplication::processEvents();
            statusBar()->showMessage("Please wait !! Data is being fetched from Database, "
                                     "this may take some time !!");
            emit getActorMovedData(scenarioBox);
            statusBar()->showMessage("Database Loaded",2000);
            QApplication::restoreOverrideCursor();
            emit getScenarioRunValues(turnSlider->value(),scenarioBox,0);
            lineCustomGraph->replot();
            emit getStateCountfromDB();

            emit getDimforBar();
            barGraphTurnSliderChanged(barGraphTurnSlider->value());

            if (prevScenario != scenarioBox)
            {
                prevScenario=scenarioBox;
                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
                return;
            }
            else
            {
                quadMapTurnSliderChanged(quadMapTurnSlider->value());
            }
            statusBar()->showMessage("Database Loaded",2000);
        }
    }
}

void MainWindow::cellSelected(QStandardItem* in)
{
    if(runButton->isEnabled())
    {
        runButton->setEnabled(false);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");
    }

    if(0==in->column())
    {
        csvAffinityModel->setHorizontalHeaderItem(in->row(),new QStandardItem(in->text()));
        csvAffinityModel->setVerticalHeaderItem(in->row(),new QStandardItem(in->text()));
    }
}

void MainWindow::xmlCellSelected(QStandardItem* in)
{
    if(runButton->isEnabled())
    {
        runButton->setEnabled(false);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");
    }

    if(0==in->column())
    {
        xmlAffinityMatrixModel->setHorizontalHeaderItem(in->row(),new QStandardItem(in->text()));
        xmlAffinityMatrixModel->setVerticalHeaderItem(in->row(),new QStandardItem(in->text()));
    }
}

void MainWindow::insertNewRowCSV()
{
    if(tableType=="NewSMPData" )
    {
        csvTableWidget->insertRow(csvTableWidget->rowCount());

        QString actorHeader;
        affinityMatrix->insertColumn(affinityMatrix->columnCount());
        affinityMatrix->insertRow(affinityMatrix->rowCount());
        actorHeader.append(" Actor ")/*.append(QString::number(affinityMatrix->columnCount()))*/;
        affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
        affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

        initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"NewSMPData");
    }
    else if(tableType=="DatabaseEdit")
    {
        csvTableWidget->insertRow(csvTableWidget->rowCount());

        QString actorHeader;
        affinityMatrix->insertColumn(affinityMatrix->columnCount());
        affinityMatrix->insertRow(affinityMatrix->rowCount());
        actorHeader.append(" Actor ")/*.append(QString::number(affinityMatrix->columnCount()))*/;
        affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
        affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

        initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"DatabaseEdit");

    }
    else if(tableType=="CSV")
    {
        modeltoCSV->insertRow(modeltoCSV->rowCount());
        int rowCount = csvAffinityModel->rowCount();

        csvAffinityModel->insertRow(csvAffinityModel->rowCount());
        csvAffinityModel->insertColumn(csvAffinityModel->columnCount());

        QString actorHeader;
        //        qDebug()<<rowCount <<"rowcount" << csvAffinityModel->rowCount();
        for(int rows = rowCount; rows < csvAffinityModel->rowCount();++ rows)
        {
            actorHeader.append(" Actor ").append(QString::number(rows+1));
            csvAffinityModel->setHorizontalHeaderItem(rows, new QStandardItem(actorHeader));
            csvAffinityModel->setVerticalHeaderItem(rows, new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(rows,"CSV");
            actorHeader.clear();
        }

    }
    else if(tableType=="XML")
    {
        xmlSmpDataModel->insertRow(xmlSmpDataModel->rowCount());

        int rowCount = xmlAffinityMatrixModel->rowCount();

        xmlAffinityMatrixModel->insertRow(xmlAffinityMatrixModel->rowCount());
        xmlAffinityMatrixModel->insertColumn(xmlAffinityMatrixModel->columnCount());

        QString actorHeader;
        //        qDebug()<<rowCount <<"rowcount" << xmlAffinityMatrixModel->rowCount();
        for(int rows = rowCount; rows < xmlAffinityMatrixModel->rowCount();++ rows)
        {
            actorHeader.append(" Actor ").append(QString::number(rows+1));
            xmlAffinityMatrixModel->setHorizontalHeaderItem(rows, new QStandardItem(actorHeader));
            xmlAffinityMatrixModel->setVerticalHeaderItem(rows, new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(rows,"XML");
            actorHeader.clear();
        }
    }
}

void MainWindow::deleteLastRow()
{
    if(tableType=="CSV")
    {
        csvAffinityModel->removeColumn(csvAffinityModel->columnCount()-1);
        csvAffinityModel->removeRow(csvAffinityModel->rowCount()-1);
        modeltoCSV->removeRow(modeltoCSV->rowCount()-1);
    }
    else if(tableType=="XML")
    {
        xmlAffinityMatrixModel->removeColumn(xmlAffinityMatrixModel->columnCount()-1);
        xmlAffinityMatrixModel->removeRow(xmlAffinityMatrixModel->rowCount()-1);
        xmlSmpDataModel->removeRow(xmlSmpDataModel->rowCount()-1);
    }
    else if(tableType=="NewSMPData" || tableType =="DatabaseEdit")
    {
        affinityMatrix->removeColumn(affinityMatrix->columnCount()-1);
        affinityMatrix->removeRow(affinityMatrix->rowCount()-1);
        csvTableWidget->removeRow(csvTableWidget->rowCount()-1);
    }
}

void MainWindow::insertNewColumnCSV()
{

    QString dimension = QInputDialog::getText(this,"Add Dimension", "Please name the new dimension");
    if(!dimension.isEmpty())
    {
        if(tableType=="NewSMPData" || tableType=="DatabaseEdit")
        {
            QTableWidgetItem * pos = new QTableWidgetItem(dimension +" Position");
            createSeperateColumn(pos);

            QTableWidgetItem * sal = new QTableWidgetItem(dimension +" Saliance");
            createSeperateColumn(sal);

        }
        else if(tableType=="CSV")
        {
            modeltoCSV->insertColumns(modeltoCSV->columnCount(),1);
            modeltoCSV->setHorizontalHeaderItem(modeltoCSV->columnCount()-1,new QStandardItem(dimension +" Position"));
            modeltoCSV->horizontalHeaderItem(modeltoCSV->columnCount()-1)->
                    setToolTip("The stated position, or advocacy, of the actor");

            modeltoCSV->insertColumns(modeltoCSV->columnCount(),1);
            modeltoCSV->setHorizontalHeaderItem(modeltoCSV->columnCount()-1,new QStandardItem(dimension +" Salience"));
            modeltoCSV->horizontalHeaderItem(modeltoCSV->columnCount()-1)->
                    setToolTip("The relative importance, or priority, for the actor");

        }
        else if(tableType=="XML")
        {
            xmlSmpDataModel->insertColumns(xmlSmpDataModel->columnCount(),1);
            xmlSmpDataModel->setHorizontalHeaderItem(xmlSmpDataModel->columnCount()-1,new QStandardItem(dimension +" Position"));
            xmlSmpDataModel->horizontalHeaderItem(xmlSmpDataModel->columnCount()-1)->
                    setToolTip("The stated position, or advocacy, of the actor");

            xmlSmpDataModel->insertColumns(xmlSmpDataModel->columnCount(),1);
            xmlSmpDataModel->setHorizontalHeaderItem(xmlSmpDataModel->columnCount()-1,new QStandardItem(dimension +" Salience"));
            xmlSmpDataModel->horizontalHeaderItem(xmlSmpDataModel->columnCount()-1)->
                    setToolTip("The relative importance, or priority, for the actor");
        }
    }
}

void MainWindow::createSeperateColumn(QTableWidgetItem * hdr)
{
    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(csvTableWidget->columnCount()-1,hdr);
    if(hdr->text()=="Position")
    {
        csvTableWidget->horizontalHeaderItem(csvTableWidget->columnCount()-1)->
                setToolTip("The stated position, or advocacy, of the actor");
    }
    else
    {
        csvTableWidget->horizontalHeaderItem(csvTableWidget->columnCount()-1)->
                setToolTip("The relative importance, or priority, for the actor");
    }
}

void MainWindow::donePushButtonClicked(bool bl)
{
    Q_UNUSED(bl)
    if(tableType=="CSV")
        saveTableViewToCSV();
    else if (tableType=="NewSMPData"|| tableType=="DatabaseEdit")
        saveAsDialog();
    else if(tableType=="XML")
        saveTableViewToXML();
}

void MainWindow::displayMessage(QString cls, QString message)
{
    QMessageBox::warning(0,cls,message);
}

void MainWindow::dockWindowChanged()
{
    if(true==quadMapDock->isVisible() && actorsName.length()>0)
    {
        quadMapTurnSliderChanged(quadMapTurnSlider->value());
    }
}

void MainWindow::setCSVItemModel(QStandardItemModel *model, QStringList scenarioName)
{
    //    emit releaseDatabase();
    lineGraphDock->setVisible(false);
    barGraphDock->setVisible(false);
    quadMapDock->setVisible(false);

    // to pass csvfile path to smp
    clearAllGraphs();
    seedRand->clear();

    savedAsXml=false;

    plotQuadMap->setEnabled(false);
    removeAllScatterPoints();

    if (stackWidget->count()>1)
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(xmlTabWidget);
    }

    for(int i=0; i <= csvTableViewTabWidget->count(); ++i)
    {
        csvTableViewTabWidget->removeTab(0);
    }

    tableType="CSV";

    csvTableView = new QTableView;
    csvTableViewTabWidget->addTab(csvTableView,"Actor Data");

    csvTableAffinityView = new QTableView;
    csvTableViewTabWidget->addTab(csvTableAffinityView, "Affinity Matrix");

    stackWidget->addWidget(csvTableViewTabWidget);
    csvTableViewTabWidget->setTabToolTip(1,"The affinity matrix records, for all pairwise comparisons of actors, "
                                           "\nthe affinity which actor i has for the position of actor j");


    csvTableView->setShowGrid(true);
    stackWidget->setCurrentIndex(1);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);
    csvTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csvTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableView(QPoint)));

    //Enable Editing
    csvTableView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    turnSlider->hide();
    tableControlsFrame->show();

    turnSlider->setVisible(false);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioDescriptionLineEdit->setEnabled(true);
    scenarioComboBox->setVisible(false);
    scenarioNameLineEdit->setVisible(true);
    scenarioNameLineEdit->setPlaceholderText("Dataset/Scenario name");
    scenarioDescriptionLineEdit->setPlaceholderText("Dataset/Scenario Description");

    actorsLineEdit->setEnabled(true);
    dimensionsLineEdit->setEnabled(true);
    addActorsPushButton->setEnabled(true);
    deleteActorsPushButton->setEnabled(true);
    dimensionsPushButton->setEnabled(true);
    donePushButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");
    donePushButton->setEnabled(true);

    setDefaultParameters();//Default Model Parameters

    modeltoCSV = model;
    //    emit getDimensionCountfromDB();

    //update: received model to widget
    csvTableView->setModel(modeltoCSV);
    csvTableView->showMaximized();
    csvTableView->resizeColumnsToContents();
    csvTableView->resizeColumnToContents(1);
    csvTableView->setWordWrap(true);

    //    csv_tableView->setAlternatingRowColors(true);
    //    csv_tableView->resizeRowsToContents();
    //    for (int c = 0; c < csv_tableView->horizontalHeader()->count(); ++c)
    //        csv_tableView->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

    connect(modeltoCSV, SIGNAL(itemChanged(QStandardItem*)),this, SLOT(cellSelected(QStandardItem*))); // disable run button

    //Enable run button, which is disabled by default
    runButton->setEnabled(true);
    runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

    //update scenario name
    mScenarioDesc.clear();
    mScenarioName.clear();
    mScenarioIds.clear();
    scenarioComboBox->clear();
    scenarioNameLineEdit->clear();

    scenarioNameLineEdit->setText(scenarioName.at(0));
    scenarioDescriptionLineEdit->clear();
    scenarioDescriptionLineEdit->setText(scenarioName.at(1));

    actorsLineEdit->setText(QString::number(modeltoCSV->rowCount()));
    dimensionsLineEdit->setText(QString::number((modeltoCSV->columnCount()-3)/2));

    modeltoCSV->horizontalHeaderItem(0)->setToolTip("An individual, institution or group");
    modeltoCSV->horizontalHeaderItem(1)->setToolTip("A description of the actor");
    modeltoCSV->horizontalHeaderItem(2)->setToolTip("The relative power or political clout for the actor");
    for(int col=3; col <modeltoCSV->columnCount()-1; ++col)
    {
        modeltoCSV->horizontalHeaderItem(col)->
                setToolTip("The stated position, or advocacy, of the actor");
        modeltoCSV->horizontalHeaderItem(++col)->
                setToolTip("The relative importance, or priority, for the actor");

    }

    csvAffinityModel = new QStandardItemModel;
    csvTableAffinityView->setModel(csvAffinityModel);
    csvTableAffinityView->setToolTip("The affinity matrix records, for all pairwise comparisons of actors, "
                                     "\nthe affinity which actor i has for the position of actor j");
    //Affinity Matrix

    while(csvAffinityModel->rowCount() != actorsLineEdit->text().toInt()
          && actorsLineEdit->text().toInt()>csvAffinityModel->rowCount())
    {
        int rowCount = csvAffinityModel->rowCount();
        if(csvAffinityModel->rowCount() < actorsLineEdit->text().toInt() ||
                csvAffinityModel->columnCount() < actorsLineEdit->text().toInt())
        {
            csvAffinityModel->insertRows(csvAffinityModel->rowCount(),(actorsLineEdit->text().toInt()
                                                                       - csvAffinityModel->rowCount()));
            csvAffinityModel->insertColumns(csvAffinityModel->columnCount(),(actorsLineEdit->text().toInt()
                                                                             - csvAffinityModel->columnCount()));
        }
        QString actorHeader;
        //        qDebug()<<rowCount <<"rowcount" << csvAffinityModel->rowCount();
        for(int rows = rowCount; rows < csvAffinityModel->rowCount();++ rows)
        {
            actorHeader=modeltoCSV->item(rows,0)->text().trimmed();
            csvAffinityModel->setHorizontalHeaderItem(rows, new QStandardItem(actorHeader));
            csvAffinityModel->setVerticalHeaderItem(rows, new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(rows,"CSV");
            actorHeader.clear();
        }
    }

    csvTableAffinityView->setContextMenuPolicy(Qt::CustomContextMenu);
    //    connect(csvTableAffinityView, SIGNAL(customContextMenuRequested(QPoint)),
    //            this, SLOT(displayCsvAffinityMenuTableView(QPoint)));

    csvTableAffinityView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    //    connect(csvAffinityModel, SIGNAL(itemChanged(QStandardItem*)),this, SLOT(cellSelected(QStandardItem*))); // disable run button

}

void MainWindow::setDBItemModelEdit(/*QSqlTableModel *modelEdit*/)
{

    if(tableType=="Database")
    {
        plotQuadMap->setEnabled(false);
        removeAllScatterPoints();

        if(stackWidget->count()>1) // 1 is csv_table view
        {
            stackWidget->removeWidget(smpDataTab);
            stackWidget->removeWidget(xmlTabWidget);
        }
        for(int i =0 ; i <= smpDataTab->count(); ++i)
        {
            smpDataTab->removeTab(0);
        }

        csvTableWidget = new QTableWidget(central);
        affinityMatrix= new QTableWidget(central);

        tableType="DatabaseEdit";

        affinityMatrix = new QTableWidget(central);
        affinityMatrix->setContextMenuPolicy(Qt::CustomContextMenu);
        affinityMatrix->setToolTip("The affinity matrix records, for all pairwise comparisons of actors, "
                                   "\nthe affinity which actor i has for the position of actor j");
        smpDataTab->addTab(csvTableWidget,"Actor Data ");
        smpDataTab->addTab(affinityMatrix," Affinity Matrix ");
        smpDataTab->setTabToolTip(1,"The affinity matrix records, for all pairwise comparisons of actors, "
                                    "\nthe affinity which actor i has for the position of actor j");

        csvTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(csvTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableWidget(QPoint)));
        connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));

        stackWidget->addWidget(smpDataTab);
        stackWidget->setCurrentIndex(1);

        removeAllGraphs();

        actorsLineEdit->setEnabled(true);
        dimensionsLineEdit->setEnabled(true);
        addActorsPushButton->setEnabled(true);
        deleteActorsPushButton->setEnabled(true);
        dimensionsPushButton->setEnabled(true);
        donePushButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");
        donePushButton->setEnabled(true);

        QString currentScenario = scenarioComboBox->currentText();

        mScenarioName.clear();
        mScenarioIds.clear();
        mScenarioDesc.clear();
        scenarioComboBox->clear();
        scenarioNameLineEdit->clear();

        scenarioNameLineEdit->setText(currentScenario);
        scenarioComboBox->setVisible(false);
        scenarioNameLineEdit->setVisible(true);
        scenarioDescriptionLineEdit->setEnabled(true);
        turnSlider->setVisible(false);
        scenarioDescriptionLineEdit->setVisible(true);
        scenarioNameLineEdit->setPlaceholderText("Dataset/Scenario name ");
        scenarioDescriptionLineEdit->setPlaceholderText("Dataset/Scenario Description ");

        csvTableWidget->resizeColumnsToContents();

        turnSlider->hide();
        tableControlsFrame->show();
        csvTableWidget->setShowGrid(true);

        for(int row = 0 ; row < actorsName.length();++row)
        {
            csvTableWidget->insertRow(row);
        }
        for(int col =0; col < 3+(dimensionsLineEdit->text().toInt())*2; ++col)
        {
            csvTableWidget->insertColumn(col);
        }

        //Headers Label
        csvTableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor"));
        csvTableWidget->horizontalHeaderItem(0)->setToolTip("An individual, institution or group");
        csvTableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Description"));
        csvTableWidget->horizontalHeaderItem(1)->setToolTip("A description of the actor");
        csvTableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Influence"));
        csvTableWidget->horizontalHeaderItem(2)->setToolTip("The relative power or political clout for the actor");

        int k=0;
        for(int i=3 ; i <csvTableWidget->columnCount(); i=i+2)
        {
            QString heading = dimensionList.at(k);
            heading = checkForHeaderString(heading);

            csvTableWidget->setHorizontalHeaderItem(i ,new QTableWidgetItem(heading +" \n Position"));
            csvTableWidget->horizontalHeaderItem(i)->
                    setToolTip("The stated position, or advocacy, of the actor");
            csvTableWidget->setHorizontalHeaderItem(i+1,new QTableWidgetItem(heading +" \n Salience"));
            csvTableWidget->horizontalHeaderItem(i+1)->
                    setToolTip("The relative importance, or priority, for the actor");
            ++k;
        }
        //Updating values
        for(int row=0 ; row < actorsName.length(); ++row)
        {
            int col=0;
            csvTableWidget->setItem(row,col,new QTableWidgetItem(actorsName.at(row)));
            csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsDescription.at(row)));
            csvTableWidget->setItem(row,++col,new QTableWidgetItem(QString::number(actorsInfl.at(row).toDouble(),'f',2)));

            for(int dim = 0 ; dim < dimensionsLineEdit->text().toInt();++ dim)
            {
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsPos[dim].at(row).toDouble()),'f',2)));
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsSal[dim].at(row).toDouble()*100),'f',2)));

            }
        }
        //Affinity Matrix
        for(int i =0 ; i < actorsLineEdit->text().toInt(); ++i)
        {
            QString actorHeader;
            actorHeader=csvTableWidget->item(i,0)->text();
            affinityMatrix->insertColumn(affinityMatrix->columnCount());
            affinityMatrix->insertRow(affinityMatrix->rowCount());
            affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
            affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

            //         initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"DatabaseEdit");
        }

        int index=0;
        for(int acti =0 ; acti < actorsName.length(); ++acti)
        {
            for ( int actj =0; actj < actorsName.length(); ++actj)
            {
                affinityMatrix->setItem(acti,actj,new QTableWidgetItem(actorAffinity.at(index++)));
            }
        }
        runButton->setEnabled(false);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");

    }
    else
    {
        displayMessage("Database File",
                       "Import a Database first, then Click on \n - Edit Database to Save as CSV");
    }
}

void MainWindow::setDBItemModel(QStandardItemModel *model)
{
    tableType="Database";
    plotQuadMap->setEnabled(true);

    if( stackWidget->count()>1 ) // 1 is csv_table view
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(xmlTabWidget);
    }

    for(int i=0; i<=csvTableViewTabWidget->count(); ++i)
    {
        csvTableViewTabWidget->removeTab(0);
    }

    csvTableView = new QTableView;
    affinityMatrix = new QTableWidget(central);

    affinityMatrix->setContextMenuPolicy(Qt::CustomContextMenu);
    //        connect(affinityMatrix, SIGNAL(customContextMenuRequested(QPoint)), this,
    //                SLOT(displayAffinityMenuTableWidget(QPoint)));
    affinityMatrix->setToolTip("The affinity matrix records, for all pairwise comparisons of actors, "
                               "\nthe affinity which actor i has for the position of actor j");
    csvTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csvTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableView(QPoint)));

    //Disable Editing
    csvTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    affinityMatrix->setEditTriggers(QAbstractItemView::NoEditTriggers);
    csvTableViewTabWidget->addTab(csvTableView, "Actor Data from DB");
    csvTableViewTabWidget->addTab(affinityMatrix," Affinity Matrix ");
    stackWidget->addWidget(csvTableViewTabWidget);
    csvTableViewTabWidget->setTabToolTip(1,"The affinity matrix records, for all pairwise comparisons of actors, "
                                           "\nthe affinity which actor i has for the position of actor j");

    stackWidget->setCurrentIndex(0);
    //    disconnect(csvTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableView(QPoint)));

    tableControlsFrame->show();

    scenarioComboBox->setEditable(false);
    scenarioNameLineEdit->setVisible(false);
    scenarioComboBox->setVisible(true);
    turnSlider->setVisible(true);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioDescriptionLineEdit->setEnabled(false);

    actorsLineEdit->setEnabled(false);
    dimensionsLineEdit->setEnabled(false);
    addActorsPushButton->setEnabled(false);
    deleteActorsPushButton->setEnabled(false);
    dimensionsPushButton->setEnabled(false);
    donePushButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");
    donePushButton->setEnabled(false);

    modeltoDB = model;
    emit getDimensionCountfromDB();

    csvTableView->setModel(modeltoDB);
    csvTableView->showMaximized();

    csvTableView->resizeColumnsToContents();
    csvTableView->hideColumn(0);
    csvTableView->setWordWrap(true);

    //Enable run button, which is disabled by default
    runButton->setEnabled(false);
    runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");

    actorsLineEdit->setText(QString::number(model->rowCount()));
    dimensionsLineEdit->setText(QString::number(dimensions+1));

    // csv-db
    for(int col = 2 ; col < (5+(dimensionsLineEdit->text().toInt())*2); ++col)
        modeltoDB->insertColumn(col);

    //Headers Label
    modeltoDB->setHeaderData(1,Qt::Horizontal,("Turn"));
    modeltoDB->setHeaderData(2,Qt::Horizontal,("Actor"));
    modeltoDB->horizontalHeaderItem(2)->setToolTip("An individual, institution or group");
    modeltoDB->setHeaderData(3,Qt::Horizontal,("Description"));
    modeltoDB->horizontalHeaderItem(3)->setToolTip("A description of the actor");
    modeltoDB->setHeaderData(4,Qt::Horizontal,("Influence"));
    modeltoDB->horizontalHeaderItem(4)->setToolTip("The relative power or political clout for the actor");

    int k=0;
    for(int i=5 ; i <modeltoDB->columnCount(); i=i+2)
    {
        QString heading = dimensionList.at(k);
        heading = checkForHeaderString(heading);

        modeltoDB->setHeaderData(i,Qt::Horizontal ,( heading +" \n Position" ));
        modeltoDB->horizontalHeaderItem(i)->
                setToolTip("The stated position, or advocacy, of the actor");
        modeltoDB->setHeaderData(i+1,Qt::Horizontal,( heading +" \n Salience" ));
        modeltoDB->horizontalHeaderItem(i+1)->setToolTip("The relative importance, or priority, for the actor");
        ++k;
    }

    //to create csv like view
    emit getActorsDesc();


    emit getInfluence(turnSlider->value());
    for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
    {
        emit getSalience(i,turnSlider->value());
        emit getPosition(i,turnSlider->value());
    }

    updateDBViewColumns();

    //Affinity Matrix
    for(int i =0 ; i < actorsLineEdit->text().toInt(); ++i)
    {
        QString actorHeader;
        actorHeader=modeltoDB->item(i,2)->text();
        affinityMatrix->insertColumn(affinityMatrix->columnCount());
        affinityMatrix->insertRow(affinityMatrix->rowCount());
        affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
        affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

    }

    int index=0;
    for(int acti =0 ; acti < actorsName.length(); ++acti)
    {
        for ( int actj =0; actj < actorsName.length(); ++actj)
        {
            affinityMatrix->setItem(acti,actj,new QTableWidgetItem(actorAffinity.at(index++)));
        }
    }
}

void MainWindow::createNewSMPData(bool bl)
{
    Q_UNUSED(bl)

    //    emit releaseDatabase();
    tableType="NewSMPData";

    clearAllGraphs();
    plotQuadMap->setEnabled(false);
    lineGraphDock->setVisible(false);
    barGraphDock->setVisible(false);
    quadMapDock->setVisible(false);

    removeAllScatterPoints();
    seedRand->clear();

    setDefaultParameters();//Default Model Parameters

    if( stackWidget->count()>1 ) // 1 is csv_table view
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(xmlTabWidget);
    }
    smpDataTab = new QTabWidget(stackWidget);

    csvTableWidget = new QTableWidget(central); // new CSV File
    connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);
    csvTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csvTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableWidget(QPoint)));

    affinityMatrix= new QTableWidget;
    affinityMatrix->setContextMenuPolicy(Qt::CustomContextMenu);
    //    connect(affinityMatrix, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayAffinityMenuTableWidget(QPoint)));
    affinityMatrix->setToolTip("The affinity matrix records, for all pairwise comparisons of actors, "
                               "\nthe affinity which actor i has for the position of actor j");

    smpDataTab->addTab(csvTableWidget,"Actor Data ");
    smpDataTab->addTab(affinityMatrix,"Affinity Matrix ");
    smpDataTab->setTabToolTip(1,"The affinity matrix records, for all pairwise comparisons of actors, "
                                "\nthe affinity which actor i has for the position of actor j");


    stackWidget->addWidget(smpDataTab);
    stackWidget->setCurrentIndex(1);

    //removeAllGraphs();

    actorsLineEdit->setEnabled(true);
    dimensionsLineEdit->setEnabled(true);
    addActorsPushButton->setEnabled(true);
    deleteActorsPushButton->setEnabled(true);
    dimensionsPushButton->setEnabled(true);
    donePushButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");
    donePushButton->setEnabled(true);

    scenarioComboBox->setVisible(false);
    scenarioNameLineEdit->setVisible(true);
    turnSlider->setVisible(false);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioDescriptionLineEdit->setEnabled(true);
    scenarioNameLineEdit->setPlaceholderText("Dataset/Scenario name");
    scenarioDescriptionLineEdit->setPlaceholderText("Dataset/Scenario Description");

    turnSlider->hide();
    tableControlsFrame->show();

    //    for (int i = 0 ; i < csv_tableWidget->columnCount(); ++i)
    //        csv_tableWidget->removeColumn(i);

    csvTableWidget->setShowGrid(true);
    csvTableWidget->setWordWrap(true);

    //csv_tableWidget->setHorizontalHeaderLabels(QString("HEADER;HEADER;HEADER;HEADER;HEADER").split(";"));

    actorsLineEdit->clear();
    dimensionsLineEdit->clear();
    mScenarioDesc.clear();
    mScenarioName.clear();
    mScenarioIds.clear();
    scenarioComboBox->clear();
    scenarioNameLineEdit->clear();
    scenarioDescriptionLineEdit->clear();

    actorsLineEdit->setText(QString::number(3));
    dimensionsLineEdit->setText(QString::number(1));

    for(int i=0;i<3;++i)
        insertNewRowCSV();

    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor"));
    csvTableWidget->horizontalHeaderItem(0)->setToolTip("An individual, institution or group");
    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Description"));
    csvTableWidget->horizontalHeaderItem(1)->setToolTip("A description of the actor");
    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Influence"));
    csvTableWidget->horizontalHeaderItem(2)->setToolTip("The relative power or political clout for the actor");

    insertNewColumnCSV();

    /*   affinityMatrix->insertColumn(affinityMatrix->columnCount());
    affinityMatrix->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor 0"));
    affinityMatrix->insertRow(affinityMatrix->rowCount());
    affinityMatrix->setVerticalHeaderItem(0,new QTableWidgetItem("Actor 0"));

    affinityMatrix->insertColumn(affinityMatrix->columnCount());
    affinityMatrix->setHorizontalHeaderItem(1,new QTableWidgetItem("Actor 1"));
    affinityMatrix->insertRow(affinityMatrix->rowCount());
    affinityMatrix->setVerticalHeaderItem(1,new QTableWidgetItem("Actor 1"));

    affinityMatrix->insertColumn(affinityMatrix->columnCount());
    affinityMatrix->setHorizontalHeaderItem(2,new QTableWidgetItem("Actor 2"));
    affinityMatrix->insertRow(affinityMatrix->rowCount());
    affinityMatrix->setVerticalHeaderItem(2,new QTableWidgetItem("Actor 2"));*/

    runButton->setEnabled(false);
    runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");

}

void MainWindow::initializeCentralViewFrame()
{
    QGroupBox * centralBox = new QGroupBox("Input Data");
    central = new QFrame;

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(central);
    centralBox->setLayout(vbox);

    central->setFrameShape(QFrame::StyledPanel);

    gLayout = new QGridLayout;
    central->setLayout(gLayout);

    tableControlsFrame = new QFrame;
    tableControlsFrame->setFrameShape(QFrame::StyledPanel);

    QGridLayout *gCLayout = new QGridLayout(tableControlsFrame);

    stackWidget = new QStackedWidget(central);

    scenarioComboBox = new QComboBox(tableControlsFrame);
    //    scenarioComboBox->setMaximumWidth(200);
    //    scenarioComboBox->setFixedWidth(150);
    gCLayout->addWidget(scenarioComboBox,0,2);
    scenarioComboBox->setEditable(true);
    scenarioComboBox->setToolTip("Enter the Scenario / Project Name");
    connect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));

    scenarioNameLineEdit = new QLineEdit(tableControlsFrame);
    scenarioNameLineEdit->setVisible(false);
    //    scenarioNameLineEdit->setMaximumWidth(200);
    //    scenarioNameLineEdit->setFixedWidth(150);
    scenarioNameLineEdit->setToolTip("Enter the Scenario / Project Name");
    gCLayout->addWidget(scenarioNameLineEdit,0,2);

    turnSlider = new QSlider(Qt::Horizontal,central);
    turnSlider->setTickInterval(1);
    turnSlider->setTickPosition(QSlider::TicksBothSides);
    turnSlider->setPageStep(1);
    turnSlider->setSingleStep(1);

    connect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));

    addActorsPushButton = new QPushButton(" + Actors",tableControlsFrame);
    addActorsPushButton->setMaximumWidth(120);
    addActorsPushButton->setFixedWidth(100);
    addActorsPushButton->setToolTip("Add an individual, institution or group");
    gCLayout->addWidget(addActorsPushButton,0,0);

    deleteActorsPushButton = new QPushButton(" - Actors",tableControlsFrame);
    deleteActorsPushButton->setMaximumWidth(120);
    deleteActorsPushButton->setFixedWidth(100);
    deleteActorsPushButton->setToolTip("Remove last individual, institution or group");
    gCLayout->addWidget(deleteActorsPushButton,0,1);

    actorsLineEdit = new QLineEdit ("3",tableControlsFrame);
    actorsLineEdit->setMaximumWidth(70);
    actorsLineEdit->setFixedWidth(60);
    actorsLineEdit->setValidator( new QIntValidator(1,1000,this));
    actorsLineEdit->setToolTip("An individual, institution or group");
    //    gCLayout->addWidget(actorsLineEdit,0,1);
    actorsLineEdit->hide();

    dimensionsPushButton = new QPushButton(" Add Dimension",tableControlsFrame);
    dimensionsPushButton->setMaximumWidth(120);
    dimensionsPushButton->setFixedWidth(100);
    dimensionsPushButton->setToolTip("Add dimensions");
    gCLayout->addWidget(dimensionsPushButton,1,0);

    dimensionsLineEdit = new QLineEdit ("1",tableControlsFrame);
    dimensionsLineEdit->setMaximumWidth(70);
    dimensionsLineEdit->setFixedWidth(60);
    dimensionsLineEdit->setValidator( new QIntValidator(1,9,this));
    dimensionsLineEdit->setToolTip("Set number of dimensions");
    //    gCLayout->addWidget(dimensionsLineEdit,1,1);
    dimensionsLineEdit->hide();

    scenarioDescriptionLineEdit = new QLineEdit(tableControlsFrame);
    scenarioDescriptionLineEdit->setToolTip("A description of the scenario or project");
    //    scenarioDescriptionLineEdit->setMaximumWidth(150);
    //    scenarioDescriptionLineEdit->setFixedWidth(130);
    gCLayout->addWidget(scenarioDescriptionLineEdit,1,2);

    donePushButton = new QPushButton("Done",tableControlsFrame);
    donePushButton->setMaximumWidth(120);
    donePushButton->setFixedWidth(100);
    donePushButton->setMaximumHeight(40);
    donePushButton->setFixedHeight(22);
    donePushButton->setToolTip("Save to new CSV / XML file");
    donePushButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");
    gCLayout->addWidget(donePushButton,1,1);

    setCentralWidget(centralBox);

    modeltoCSV = new QStandardItemModel;
    modeltoDB = new QStandardItemModel;

    csvTableViewTabWidget = new QTabWidget(central);
    //  csvTableView = new QTableView; // CSV and DB
    //    csvTableAffinityView = new QTableView;

    //    csvTableViewTabWidget->addTab(csvTableView,"Actor Data");
    //    csvTableView->setShowGrid(true);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);

    smpDataTab = new QTabWidget(central);
    csvTableWidget = new QTableWidget(central);
    affinityMatrix = new QTableWidget(central);

    smpDataTab->addTab(csvTableWidget,"Actor Data ");
    smpDataTab->addTab(affinityMatrix," Affinity Matrix ");

    // stackWidget->addWidget(smpDataTab);
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);
    connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));
    xmlTabWidget = new QTabWidget(central); //Xml and Affinity Matrix

    gLayout->addWidget(tableControlsFrame);
    gLayout->addWidget(turnSlider);
    gLayout->addWidget(stackWidget);

    stackWidget->addWidget(csvTableViewTabWidget);
    stackWidget->addWidget(smpDataTab);
    stackWidget->addWidget(xmlTabWidget);

    tableControlsFrame->hide();
    turnSlider->hide();

    connect(addActorsPushButton,SIGNAL(pressed()),this, SLOT(insertNewRowCSV()));
    connect(deleteActorsPushButton,SIGNAL(pressed()),this, SLOT(deleteLastRow()));
    connect(dimensionsPushButton,SIGNAL(pressed()),this, SLOT(insertNewColumnCSV()));
    connect(donePushButton,SIGNAL(clicked(bool)),this, SLOT(donePushButtonClicked(bool)));

    dimension=0;
    in=0;
    generateColors();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About KTAB SMP"),
                       tr("KTAB SMP\n\nVersion 1.0\n\n"
                          "KTAB is an open-source toolkit for assembling models that allow "
                          "systematic and rigorous analysis of Collective Decision-Making "
                          "Processes (CDMPs).  KTAB is intended to be a platform that contains "
                          "a number of models that can simulate CDMPs.  The initial model that "
                          "has been instantiated in KTAB is called the Spatial Model of Politics(SMP)."
                          "\n \n"
                          "More information can be found at http://kapsarc.github.io/KTAB/"
                          "\n \n"
                          "Copyright © KAPSARC"
                          "\n \n"
                          "KTAB is released as open source software under the terms of the MIT License (Expat)"
                          "\n \n"
                          "Please email comments, bug reports, and any feedback to ktab@kapsarc.org"));

}

void MainWindow::chooseActorColors()
{
    if(actorsName.length()>0 && tableType=="Database")
    {
        QVector<QColor> colors;

        for(int i=0; i < actorsName.length(); ++i)
            colors.append(colorsList.at(i));

        ColorPickerDialog *colorPicker = new ColorPickerDialog;
        connect(colorPicker,SIGNAL(changedColors(QVector<QColor>)),this,SLOT(updateColors(QVector<QColor>)));
        colorPicker->intializeActors(actorsName,colors);
        colorPicker->show();
    }
    else
        displayMessage("Actors color picker","Please Import DB or RUN SMP Model");

}

void MainWindow::importActorColors()
{
    if(actorsName.length()>0 && tableType=="Database")
    {
        QString colorCodeCsvFilePth;
        colorCodeCsvFilePth = QFileDialog::getOpenFileName(this,tr("Open CSV File"), homeDirectory , tr("CSV File (*.csv)"));

        //emit path to csv class for processing
        if(!colorCodeCsvFilePth.isEmpty())
        {
            QDir dir =QFileInfo(colorCodeCsvFilePth).absoluteDir();
            homeDirectory = dir.absolutePath();
            setCurrentFile(colorCodeCsvFilePth);
            emit importColors(colorCodeCsvFilePth,actorsName.length());
        }
    }
    else
        displayMessage("Actors color picker","Please Import DB or RUN SMP Model");
}

void MainWindow::exportActorColors()
{
    if(actorsName.length()>0 && tableType=="Database")
    {
        QString colorPaletteCsvFileNameLocation = QFileDialog::getSaveFileName(
                    this, tr("Save Log File to "),homeDirectory,"CSV File (*.csv)");
        if(!colorPaletteCsvFileNameLocation.isEmpty())
        {
            QDir dir =QFileInfo(colorPaletteCsvFileNameLocation).absoluteDir();
            homeDirectory = dir.absolutePath();
        }
        if(!colorPaletteCsvFileNameLocation.endsWith(".csv"))
            colorPaletteCsvFileNameLocation.append(".csv");

        setCurrentFile(colorPaletteCsvFileNameLocation);

        QVector<int> actorIdList;
        QVector<QString> actorColorsList;
        for(int act=0; act < actorsName.length(); ++act)
        {
            actorIdList.append(act);
            actorColorsList.append(colorsList.at(act).name());
        }
        emit exportColors(colorPaletteCsvFileNameLocation,actorIdList,actorColorsList);

    }
    else
        displayMessage("Actors color picker","Please Import DB or RUN SMP Model");
}

void MainWindow::resetActorColors()
{
    if(actorsName.length()>0 && tableType=="Database")
    {
        generateColors();
        changesActorsStyleSheet();
    }
    else
        displayMessage("Actors color picker","Please Import DB or RUN SMP Model");
}

void MainWindow::updateColors(QVector<QColor> updatedColors)
{
    for(int i=0; i < updatedColors.length(); ++i)
        colorsList[i]=updatedColors.at(i);

    changesActorsStyleSheet();
}

void MainWindow::changesActorsStyleSheet()
{
    for(int actId=0; actId < actorsName.length(); ++actId)
    {
        QColor mycolor =colorsList.at(actId);

        QString style = "background: rgb(%1, %2, %3);";
        style = style.arg(mycolor.red()).arg(mycolor.green()).arg(mycolor.blue());
        style += "color:white; font-size:15px;";
        style += "font-weight:bold;";

        barGraphActorsCheckBoxList.at(actId)->setStyleSheet(style);
        lineGraphActorsCheckBoxList.at(actId)->setStyleSheet(style);
        quadMapReceiversCheckBoxList.at(actId)->setStyleSheet(style);
        quadMapInitiatorsRadioButtonList.at(actId)->setStyleSheet(style);

        lineLabelList.at(actId)->setColor(colorsList.at(actId));
    }
    turnSlider->valueChanged(turnSlider->value());
    barGraphTurnSlider->valueChanged(barGraphTurnSlider->value());
}

void MainWindow::createActions()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    QList <QKeySequence> seq;
    fileMenu->setToolTipsVisible(true);

    const QIcon NewSMPDataIcon = QIcon::fromTheme("Create New Actor Data", QIcon("://images/create.png"));
    QAction *NewSMPDataAct = new QAction(NewSMPDataIcon, tr("&Create New Actor Data"), this);
    NewSMPDataAct->setShortcuts(QKeySequence::New);
    NewSMPDataAct->setStatusTip(tr("Create New Actor Data"));
    connect(NewSMPDataAct, SIGNAL(triggered(bool)), this,SLOT(createNewSMPData(bool)));
    fileMenu->addAction(NewSMPDataAct);
    fileToolBar->addAction(NewSMPDataAct);

    const QIcon csvIcon = QIcon::fromTheme("View/Modify Exisiting CSV File", QIcon("://images/editcsv.png"));
    QAction *readCsvAct = new QAction(csvIcon, tr("&View/Modify Exisiting CSV File"), this);
    readCsvAct->setShortcuts(QKeySequence::Open);
    readCsvAct->setStatusTip(tr("View/Modify Exisiting CSV File"));
    connect(readCsvAct, SIGNAL(triggered(bool)), this,SLOT(csvGetFilePAth(bool)));
    fileMenu->addAction(readCsvAct);
    fileToolBar->addAction(readCsvAct);

    seq.clear();
    seq.append(Qt::Key_X | Qt::CTRL);
    const QIcon impXmlIcon = QIcon::fromTheme("View/Modify Exisiting Xml File ", QIcon("://images/xml.png"));
    QAction *importXmlAct = new QAction(impXmlIcon, tr("&View/Modify Exisiting Xml File"), this);
    importXmlAct->setShortcuts(seq);
    importXmlAct->setStatusTip(tr("View/Modify Exisiting Xml File"));
    connect(importXmlAct, SIGNAL(triggered(bool)), this,SLOT(importXmlGetFilePath(bool)));
    fileMenu->addAction(importXmlAct);
    fileToolBar->addAction(importXmlAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon("://images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save..."), this);
    connect(saveAct, SIGNAL(triggered(bool)), this, SLOT(donePushButtonClicked(bool)));
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save"));
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    fileToolBar->addSeparator();
    fileMenu->addSeparator();

    seq.clear();
    seq.append(Qt::Key_I | Qt::CTRL);
    const QIcon dbConfig = QIcon::fromTheme("Configure Database ", QIcon("://images/configdb.png"));
    QAction *configureDBAct = new QAction(dbConfig, tr("&Configure Database"), this);
    configureDBAct->setShortcuts(seq);
    configureDBAct->setToolTip("Configure a database");
    configureDBAct->setStatusTip(tr("Configure a database"));
    connect(configureDBAct,SIGNAL(triggered(bool)),this,SLOT(configUsingMenu(bool)));
    connect(configureDBAct, SIGNAL(triggered(bool)),this,SLOT(configureDB(bool)));
    fileMenu->addAction(configureDBAct);
    fileToolBar->addAction(configureDBAct);

    seq.clear();
    seq.append(Qt::Key_I | Qt::CTRL);
    const QIcon dbIcon = QIcon::fromTheme("Import Database ", QIcon("://images/import.png"));
    QAction *importDBAct = new QAction(dbIcon, tr("&Import Database"), this);
    importDBAct->setShortcuts(seq);
    importDBAct->setToolTip("Import a database file of a previous run");
    importDBAct->setStatusTip(tr("Import a database file of a previous run"));
    connect(importDBAct, SIGNAL(triggered(bool)),this,SLOT(dbImported(bool)));
    connect(importDBAct, SIGNAL(triggered(bool)), this,SLOT(dbGetFilePAth(bool)));
    fileMenu->addAction(importDBAct);
    fileToolBar->addAction(importDBAct);

    seq.clear();
    seq.append(Qt::Key_E | Qt::CTRL);
    const QIcon modDBIcon = QIcon::fromTheme("Edit Database", QIcon("://images/editdb.png"));
    QAction *modifyDBAct = new QAction(modDBIcon, tr("&Edit DB, Save As"), this);
    modifyDBAct->setShortcuts(seq);
    modifyDBAct->setToolTip("Edit a database file of a previous run and \n save the data or model parameters");
    modifyDBAct->setStatusTip(tr("Edit a database file of a previous run and save the data or model parameters"));
    connect(modifyDBAct, SIGNAL(triggered(bool)), this,SLOT(dbEditGetFilePAth(bool)));
    fileMenu->addAction(modifyDBAct);
    fileToolBar->addAction(modifyDBAct);

    for (int i = 0; i < maxRecentFilesCount; ++i)
    {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),this, SLOT(openRecentFile()));
    }

    fileToolBar->addSeparator();
    separatorAct = fileMenu->addSeparator();
    for (int i = 0; i < maxRecentFilesCount; ++i)
    {
        fileMenu->addAction(recentFileActs[i]);
    }
    fileMenu->addSeparator();
    updateRecentFileActions();

    fileMenu->addSeparator();

    QAction *clearFileHistoryAct = new QAction(tr("Clear Recent File History"), this);
    clearFileHistoryAct->setToolTip("Clear Recently Open File History");
    clearFileHistoryAct->setStatusTip(tr("Clear Recently Open File History"));
    connect(clearFileHistoryAct, SIGNAL(triggered(bool)), this,SLOT(clearRecentFile(bool)));
    fileMenu->addAction(clearFileHistoryAct);
    fileMenu->addSeparator();

    fileMenu->addSeparator();

    QAction *changeHomeAct = new QAction(tr("Change Home Directory"), this);
    changeHomeAct->setToolTip("Change Home Directory");
    changeHomeAct->setStatusTip(tr("Change Home Directory"));
    connect(changeHomeAct, SIGNAL(triggered(bool)), this,SLOT(changeHomeDirectory(bool)));
    fileMenu->addAction(changeHomeAct);
    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("Quit ", QIcon("://images/exit.png"));
    QAction *quitAct = new QAction(exitIcon, tr("&Quit"), this);
    connect(quitAct, SIGNAL(triggered(bool)), this,SLOT(close()));
    fileMenu->addAction(quitAct);
    quitAct->setShortcuts(QKeySequence::Quit);
    quitAct->setStatusTip(tr("Quit the application"));

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->setToolTipsVisible(true);

    menuBar()->addSeparator();

    QMenu *optionMenu = menuBar()->addMenu(tr("&Log Options"));
    optionMenu->setToolTipsVisible(true);

    logActions = new QActionGroup(this);
    logActions->setExclusive(true);

    logDefaultAct =new QAction(tr("&Default"), this);
    logDefaultAct->setCheckable(true);
    optionMenu->addAction(logDefaultAct);
    logDefaultAct->setToolTip("Record the SMP model log in a timestamp-named file");
    logDefaultAct->setStatusTip(tr("Record the SMP model log in a timestamp-named file"));
    logActions->addAction(logDefaultAct);
    logDefaultAct->setChecked(true);

    logNewAct =new QAction(tr("&Custom"), this);
    logNewAct->setCheckable(true);
    optionMenu->addAction(logNewAct);
    logNewAct->setToolTip("Record the SMP model log in a specific file / location");
    logNewAct->setStatusTip(tr("Record the SMP model log in a specific file / location"));
    logActions->addAction(logNewAct);

    logNoneAct =new QAction(tr("&None"), this);
    logNoneAct->setCheckable(true);
    optionMenu->addAction(logNoneAct);
    logNoneAct->setToolTip(tr("Disable logging of the SMP model run"));
    logNoneAct->setStatusTip(tr("Disable logging of the SMP model run"));
    logActions->addAction(logNoneAct);

    menuBar()->addSeparator();

    QMenu *actorColors = menuBar()->addMenu(tr("&Colors Options"));
    actorColors->setToolTipsVisible(true);

    const QIcon colorIcon = QIcon::fromTheme("ColorPicker ", QIcon("://images/colorpicker.png"));
    QAction *colorAct =new QAction(colorIcon,tr("&Change Actor Colors"), this);
    actorColors->addAction(colorAct);
    colorAct->setToolTip("Change actor colors displayed in charts");
    connect(colorAct, SIGNAL(triggered(bool)),this,SLOT(chooseActorColors()));
    colorAct->setStatusTip(tr("Pick colors for Actors"));

    QAction *importActorColorAct =new QAction(tr("&Import Actor Colors"), this);
    actorColors->addAction(importActorColorAct);
    importActorColorAct->setToolTip("Import actor colors from CSV");
    connect(importActorColorAct, SIGNAL(triggered(bool)),this,SLOT(importActorColors()));
    importActorColorAct->setStatusTip(tr("Import Actor colors from CSV"));

    QAction *exportActorColorAct =new QAction(tr("&Export Actor Colors"), this);
    actorColors->addAction(exportActorColorAct);
    exportActorColorAct->setToolTip("Export actor colors to CSV");
    connect(exportActorColorAct, SIGNAL(triggered(bool)),this,SLOT(exportActorColors()));
    exportActorColorAct->setStatusTip(tr("Export Actor colors to CSV"));

    QAction *resetActorColorAct =new QAction(tr("&Reset Actor Colors"), this);
    actorColors->addAction(resetActorColorAct);
    resetActorColorAct->setToolTip("Reset colors to default");
    connect(resetActorColorAct, SIGNAL(triggered(bool)),this,SLOT(resetActorColors()));
    resetActorColorAct->setStatusTip(tr("Reset Actor colors to Default"));

    fileToolBar->addAction(colorAct);

    menuBar()->addSeparator();
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *aboutAct =new QAction(tr("&About"), this);
    helpMenu->addAction(aboutAct);
    connect(aboutAct, SIGNAL(triggered(bool)),this,SLOT(about()));
    aboutAct->setStatusTip(tr("About SMPQ "));

}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::createLinePlotsDockWindows()
{
    lineGraphDock = new QDockWidget(tr("Line Plots"), this);
    lineGraphMainFrame = new QFrame(lineGraphDock);
    lineGraphGridLayout = new QGridLayout(lineGraphMainFrame);

    //draw a graph
    initializeLineGraphDock();

    lineGraphDock->setWidget(lineGraphMainFrame);
    lineGraphDock->setToolTip("The Line Plot displays the actors' \n changing Positions by turn");
    addDockWidget(Qt::BottomDockWidgetArea, lineGraphDock);
    viewMenu->addAction(lineGraphDock->toggleViewAction());
    viewMenu->actions().at(1)->setToolTip("Show/hide the Line Plot displaying \n the positions of all actors in all turns");
    viewMenu->actions().at(1)->setStatusTip("Show/hide the Line Plot displaying the positions of all actors in all turns");
    connect(lineGraphDock,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(dockWindowChanged()));
    lineGraphDock->setVisible(false);
}

void MainWindow::createBarPlotsDockWindows()
{
    barGraphDock = new QDockWidget(tr("Bar Plots"), this);
    barGraphMainFrame = new QFrame(barGraphDock);
    barGraphGridLayout = new QGridLayout(barGraphMainFrame);

    //initialize BarGraph layout
    initializeBarGraphDock();

    barGraphDock->setWidget(barGraphMainFrame);
    barGraphDock->setToolTip("The Bar Plot displays the Effective Power \n and Positions of actors for each turn");

    addDockWidget(Qt::BottomDockWidgetArea, barGraphDock);
    viewMenu->addAction(barGraphDock->toggleViewAction());
    viewMenu->actions().at(2)->setToolTip("Show/hide the Bar Plot displaying the position of all actors in each turn");
    viewMenu->actions().at(2)->setStatusTip("Show/hide the Bar Plot displaying the position of all actors in each turn");

    connect(barGraphDock,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(dockWindowChanged()));
    barGraphDock->setVisible(false);
}

void MainWindow::createQuadMapDockWindows()
{
    quadMapDock = new QDockWidget(tr("QuadMap"), this);
    quadMapMainFrame = new QFrame(quadMapDock);
    quadMapGridLayout = new QGridLayout(quadMapMainFrame);

    //initialize QuadMap layout
    initializeQuadMapDock();

    quadMapDock->setWidget(quadMapMainFrame);
    quadMapDock->setToolTip("This chart displays the expected utility changes for a \nselected actor challenging all other actors");

    addDockWidget(Qt::BottomDockWidgetArea, quadMapDock);
    viewMenu->addAction(quadMapDock->toggleViewAction());
    viewMenu->actions().at(3)->setToolTip("Show/hide the Quad Map displaying the expected"
                                          "\n utility changes from an actor challenging others");
    viewMenu->actions().at(3)->setStatusTip("Show/hide the Quad Map displaying the expected"
                                            " utility changes from an actor challenging others");



    connect(quadMapDock,SIGNAL(visibilityChanged(bool)),this,SLOT(dockWindowChanged()));

    quadMapDock->setVisible(false);

}

void MainWindow::createModelParametersDockWindow()
{
    modelParametersDock = new QDockWidget(tr("Model Parameters"),this);
    initializeModelParametersDock();
}

void MainWindow::saveTableViewToCSV()
{
    savedAsXml=false;
    if(0==validateControlButtons("csv_tableView"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File to "),homeDirectory,tr("CSV File (*.csv)"));
        if(!saveFilePath.isEmpty())
        {
            if( !saveFilePath.endsWith(".csv") )
                saveFilePath.append(".csv");

            QDir dir =QFileInfo(saveFilePath).absoluteDir();
            homeDirectory = dir.absolutePath();

            // to pass csvfile path to smp
            csvPath = saveFilePath;

            QFile f(saveFilePath);

            if (f.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream data( &f );
                QStringList strList;

                strList <<scenarioNameLineEdit->text();
                strList <<scenarioDescriptionLineEdit->text().trimmed();
                strList <<QString::number(modeltoCSV->rowCount());
                strList <<QString::number((modeltoCSV->columnCount()-3)/2);

                data << strList.join(",") << ","<< "\n";
                strList.clear();

                //Appending a header
                for( int col = 0; col < modeltoCSV->columnCount(); ++col )
                {
                    strList <<modeltoCSV->horizontalHeaderItem(col)->data(Qt::DisplayRole).toString().remove("\n");
                }
                data << strList.join(",") << "," <<"\n";

                //appending data
                for (int row=0; row<modeltoCSV->rowCount(); row++)
                {
                    strList.clear();

                    for (int col=0; col<modeltoCSV->columnCount(); col++)
                    {
                        strList << modeltoCSV->data(modeltoCSV->index(row,col)).toString();
                    }
                    data << strList.join(",") + "\n";
                }
                f.close();
                setCurrentFile(saveFilePath);
                runButton->setEnabled(true);
                runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

            }
        }
    }
}

void MainWindow::saveAsDialog()
{
    QMessageBox msgBox;
    msgBox.setText("SMP ACTOR DATA !");
    msgBox.setInformativeText("How do you want to save your data?");
    QAbstractButton *csvButton = msgBox.addButton(trUtf8("CSV"), QMessageBox::YesRole);
    QAbstractButton *xmlButton = msgBox.addButton(trUtf8("XML"), QMessageBox::YesRole);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.exec();

    if(msgBox.clickedButton() == csvButton)
    {
        savedAsXml=false;
        saveTableWidgetToCSV(true);
    }
    else if(msgBox.clickedButton() ==  xmlButton)
    {
        savedAsXml=true;
        saveTableWidgetToXML(true);
    }

}

void MainWindow::saveTableWidgetToCSV(bool bl)
{
    savedAsXml=false;
    if(0==validateControlButtons("csv_tableWidget"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File As"),homeDirectory,tr("CSV files (*.csv)"));
        if(!saveFilePath.isEmpty())
        {

            QDir dir =QFileInfo(saveFilePath).absoluteDir();
            homeDirectory = dir.absolutePath();

            if( !saveFilePath.endsWith(".csv") )
                saveFilePath.append(".csv");

            // to pass csvfile path to smp
            csvPath = saveFilePath;

            QFile f(saveFilePath);

            if (f.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream data( &f );
                QStringList strList;

                strList <<scenarioNameLineEdit->text();
                strList <<scenarioDescriptionLineEdit->text();
                strList <<QString::number(csvTableWidget->rowCount());
                strList <<QString::number((csvTableWidget->columnCount()-3)/2);

                data << strList.join(",") << ","<< "\n";
                strList.clear();

                //Appending a header
                for( int col = 0; col < csvTableWidget->columnCount(); ++col )
                {
                    strList << csvTableWidget->horizontalHeaderItem(col)->data(Qt::DisplayRole).toString().remove("\n");
                }
                data << strList.join(",") << "," <<"\n";

                for( int row = 0; row < csvTableWidget->rowCount(); ++row )
                {
                    strList.clear();
                    for( int column = 0; column < csvTableWidget->columnCount(); ++column )
                    {
                        strList <<csvTableWidget->item(row,column)->text();
                    }
                    data << strList.join( "," ) << ","  << "\n";
                }
                f.close();
                setCurrentFile(saveFilePath);
                runButton->setEnabled(true);
                runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

            }
        }
    }
}

void MainWindow::saveTableWidgetToXML(bool bl)
{
    savedAsXml=true;
    if(0==validateControlButtons("csv_tableWidget"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File As"),homeDirectory,tr("XML files (*.xml)"));
        if(!saveFilePath.isEmpty())
        {
            if( !saveFilePath.endsWith(".xml"))
                saveFilePath.append(".xml");

            QDir dir =QFileInfo(saveFilePath).absoluteDir();
            homeDirectory = dir.absolutePath();

            // to pass xml path to smp
            xmlPath = saveFilePath;

            QStringList parameters;
            parameters.append(xmlPath);
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

            for(int i =3; i < csvTableWidget->columnCount(); i += 2)
            {
                parameters.append(csvTableWidget->horizontalHeaderItem(i)
                                  ->data(Qt::DisplayRole).toString());
            }
            emit saveNewSMPDataToXMLFile(parameters,csvTableWidget,affinityMatrix);
            setCurrentFile(saveFilePath);
            runButton->setEnabled(true);
            runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

        }
    }
}

int MainWindow::validateControlButtons(QString viewName)
{
    int ret=0;

    if(true==scenarioNameLineEdit->text().isEmpty())
    {
        ++ret;
        displayMessage("Dataset/Scenario", "Enter Dataset/Scenario name");
    }

    if(true==scenarioDescriptionLineEdit->text().isEmpty())
    {
        ++ret;
        displayMessage("Dataset/Scenario Description", "Enter Dataset/Scenario Description");
    }

    if("csv_tableWidget"==viewName)
    {
        for(int rowIndex = 0; rowIndex < csvTableWidget->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < csvTableWidget->columnCount(); ++colIndex)
            {
                QTableWidgetItem* item = csvTableWidget->item(rowIndex,colIndex);

                if( !item  || item->text().isEmpty() )
                {
                    if(colIndex!=0)
                    {
                        QTableWidgetItem* actorName = csvTableWidget->item(rowIndex,0);// actor name
                        QTableWidgetItem* columnHeaderName = csvTableWidget->horizontalHeaderItem(colIndex); //column hdr
                        QMessageBox::about(0,"No Data Entered",
                                           "Please Enter \""  +columnHeaderName->text() +
                                           "\" Data for Actor \""+ actorName->text() +
                                           "\" at Row : " + QString::number(rowIndex+1));
                    }
                    else
                    {
                        QMessageBox::about(0,"No Data Entered",
                                           "please Enter Actor Name at Row : "+
                                           QString::number(rowIndex+1));
                    }
                    return ++ret;
                }
                else
                {
                    if(colIndex==0)
                    {
                        if(csvTableWidget->item(rowIndex,0)->text().length()>25)// actor name
                        {
                            QMessageBox::about(0,"Actor Name",
                                               "Actor Name can be a maximum of 25 characters, Row : "+
                                               QString::number(rowIndex+1));
                            return ++ret;
                        }
                    }
                    else if(colIndex==1)
                    {
                        if(csvTableWidget->item(rowIndex,1)->text().length()>256)// actor name
                        {
                            QTableWidgetItem* actorName = csvTableWidget->item(rowIndex,0);// actor name
                            QTableWidgetItem* columnHeaderName = csvTableWidget->horizontalHeaderItem(colIndex); //column hdr
                            QMessageBox::about(0,columnHeaderName->text() +" Length",
                                               "The \""  +columnHeaderName->text() +
                                               "\" length can be a maximum of "
                                               "256 characters for \""+ actorName->text() +
                                               "\" at Row : " + QString::number(rowIndex+1));

                            return ++ret;
                        }
                    }
                }
            }
        }
    }
    else if("csv_tableView"==viewName)
    {
        for(int rowIndex = 0; rowIndex < modeltoCSV->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < modeltoCSV->columnCount(); ++colIndex)
            {
                if(modeltoCSV->data(modeltoCSV->index(rowIndex,colIndex)).toString().isEmpty())
                {
                    if(colIndex!=0)
                    {
                        QString actorName = modeltoCSV->data(modeltoCSV->index(rowIndex,0)).toString();// actor name
                        QString columnHeaderName = modeltoCSV->headerData(colIndex,Qt::Horizontal).toString();
                        QMessageBox::about(0,"No Data Entered",
                                           "Please Enter \""  +columnHeaderName +
                                           "\" Data for Actor \""+ actorName +
                                           "\" at Row : " + QString::number(rowIndex+1));
                    }
                    else
                    {
                        QMessageBox::about(0,"No Data Entered",
                                           "please Enter Actor Name at Row : "+
                                           QString::number(rowIndex+1));
                    }
                    return ++ret;
                }
                else
                {
                    if(colIndex==0)
                    {
                        if(modeltoCSV->data(modeltoCSV->index(rowIndex,0)).toString().length()>25)// actor name
                        {
                            QMessageBox::about(0,"Actor Name",
                                               "Actor Name can be a maximum of 25 characters, Row : "+
                                               QString::number(rowIndex+1));
                            return ++ret;
                        }
                    }
                    else if(colIndex==1)
                    {
                        if(modeltoCSV->data(modeltoCSV->index(rowIndex,1)).toString().length()>256)// actor name
                        {
                            QString actorName = modeltoCSV->data(modeltoCSV->index(rowIndex,0)).toString();// actor name
                            QString columnHeaderName = modeltoCSV->headerData(colIndex,Qt::Horizontal).toString();
                            QMessageBox::about(0,columnHeaderName +" Length",
                                               "The \""  +columnHeaderName +
                                               "\" length can be a maximum of "
                                               "256 characters for \""+ actorName +
                                               "\" at Row : " + QString::number(rowIndex+1));

                            return ++ret;
                        }
                    }
                }
            }
        }
    }
    else if("xml_tableView"==viewName)
    {
        for(int rowIndex = 0; rowIndex < xmlSmpDataModel->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < xmlSmpDataModel->columnCount(); ++colIndex)
            {
                if(xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,colIndex)).toString().isEmpty())
                {
                    if(colIndex!=0)
                    {
                        QString actorName = xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,0)).toString();// actor name
                        QString columnHeaderName = xmlSmpDataModel->headerData(colIndex,Qt::Horizontal).toString();
                        QMessageBox::about(0,"No Data Entered",
                                           "Please Enter \""  +columnHeaderName +
                                           "\" Data for Actor \""+ actorName +
                                           "\" at Row : " + QString::number(rowIndex+1));
                    }
                    else
                    {
                        QMessageBox::about(0,"No Data Entered",
                                           "please Enter Actor Name at Row : "+
                                           QString::number(rowIndex+1));
                    }
                    return ++ret;
                }
                else
                {
                    if(colIndex==0)
                    {
                        if(xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,0)).toString().length()>25)// actor name
                        {
                            QMessageBox::about(0,"Actor Name",
                                               "Actor Name can be a maximum of 25 characters, Row : "+
                                               QString::number(rowIndex+1));
                            return ++ret;
                        }
                    }
                    else if(colIndex==1)
                    {
                        if(xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,1)).toString().length()>256)// actor name
                        {
                            QString actorName = xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,0)).toString();// actor name
                            QString columnHeaderName = xmlSmpDataModel->headerData(colIndex,Qt::Horizontal).toString();
                            QMessageBox::about(0,columnHeaderName +" Length",
                                               "The \""  +columnHeaderName +
                                               "\" length can be a maximum of "
                                               "256 characters for \""+ actorName +
                                               "\" at Row : " + QString::number(rowIndex+1));
                            return ++ret;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

QString MainWindow::getImageFileName(QString imgType, QString imgFileName, QString imgExt)
{
    QString imgFilePath = QFileDialog::getSaveFileName(this, tr("Save Image File to "),QString(homeDirectory+QDir::separator()+imgFileName),
                                                       imgType);
    if(!imgFilePath.isEmpty())
    {
        if(!imgFilePath.endsWith(imgExt))
            imgFilePath.append(imgExt);

        QDir dir =QFileInfo(imgFilePath).absoluteDir();
        homeDirectory = dir.absolutePath();

    }
    return imgFilePath;
}

void MainWindow::updateDBViewColumns()
{
    //Updating values
    for(int row=0 ; row < actorsName.length(); ++row)
    {
        int col=2;

        if(!actorsInfl.isEmpty()) // To make Sure App doesnt crash when there is no data
        {
            modeltoDB->setItem(row,col,new QStandardItem(actorsName.at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(actorsDescription.at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(
                                   QString::number(actorsInfl.at(row).toDouble(),'f',2)));

            for(int dim = 0 ; dim < dimensionsLineEdit->text().toInt();++ dim)
            {
                modeltoDB->setItem(row,++col,new QStandardItem(
                                       QString::number(actorsPos[dim].at(row).toDouble(),'f',2)));
                modeltoDB->setItem(row,++col,new QStandardItem(
                                       QString::number(actorsSal[dim].at(row).toDouble()*100,'f',2)));

            }

        }
    }
}

QString MainWindow::checkForHeaderString(QString header)
{
    if(header.contains("Position"))
        header.replace("Position","");
    else if(header.contains("position"))
        header.replace("position","");

    if(header.contains("Salience"))
        header.replace("Salience","");
    else if(header.contains("salience"))
        header.replace("salience","");

    return header.trimmed();
}

void MainWindow::displayMenuTableWidget(QPoint pos)
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
    QAction *act = menu.exec(csvTableWidget->viewport()->mapToGlobal(pos));

    if (act == col)
    {
        if(csvTableWidget->currentColumn()>2)
            csvTableWidget->removeColumn(csvTableWidget->currentColumn());
        else
            statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
    }
    if (act == row)
    {
        affinityMatrix->removeRow(csvTableWidget->currentRow());
        affinityMatrix->removeColumn(csvTableWidget->currentRow());
        csvTableWidget->removeRow(csvTableWidget->currentRow());

    }
    if (act == rename)
    {
        if(csvTableWidget->currentColumn()>2)
        {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                 tr("Header Name"), QLineEdit::Normal,
                                                 csvTableWidget->horizontalHeaderItem(csvTableWidget->currentColumn())->text(), &ok);

            if (ok && !text.isEmpty())
            {
                if(csvTableWidget->currentColumn()%2!=0)
                {
                    if(!(text.contains("Position") || text.contains("position")))
                    {
                        if(!text.contains("\n"))
                            text = text.append(" \n Position");
                        else
                            text = text.append(" Position");
                    }
                    else
                    {
                        QString header = text;
                        header.remove("Position").remove("position");
                        header.remove("\n");
                        header.append("\n Position");
                        text = header;
                    }
                    csvTableWidget->horizontalHeaderItem(csvTableWidget->currentColumn())->
                            setToolTip("The stated position, or advocacy, of the actor");
                }
                else
                {
                    if(!(text.contains("Salience") || text.contains("salience")))
                    {
                        if(!text.contains("\n"))
                            text = text.append(" \n Salience");
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
                    csvTableWidget->horizontalHeaderItem(csvTableWidget->currentColumn())->
                            setToolTip("The relative importance, or priority, for the actor");
                }
                csvTableWidget->setHorizontalHeaderItem(csvTableWidget->currentColumn(),new QTableWidgetItem(text));

                statusBar()->showMessage("Header changed");
            }
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if (act == posCol)
    {
        if(csvTableWidget->currentColumn()>2)
        {
            csvTableWidget->insertColumn(csvTableWidget->currentColumn());
            csvTableWidget->setHorizontalHeaderItem(csvTableWidget->currentColumn()-1,new QTableWidgetItem("Position"));
            csvTableWidget->horizontalHeaderItem(csvTableWidget->currentColumn()-1)->
                    setToolTip("The stated position, or advocacy, of the actor");
            statusBar()->showMessage("Position Column Inserted");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot change"
                                     " Actor, Description and Influence Columns");
    }

    if (act == salCol)
    {
        if(csvTableWidget->currentColumn()>2)
        {
            csvTableWidget->insertColumn(csvTableWidget->currentColumn());
            csvTableWidget->setHorizontalHeaderItem(csvTableWidget->currentColumn()-1,new QTableWidgetItem("Salience"));
            csvTableWidget->horizontalHeaderItem(csvTableWidget->currentColumn()-1)->
                    setToolTip("The relative importance, or priority, for the actor");
            statusBar()->showMessage("Salience Column Inserted");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot change"
                                     " Actor, Description and Influence Columns");
    }

    if(act == newRow)
    {
        affinityMatrix->insertRow(csvTableWidget->currentRow());
        affinityMatrix->insertColumn(csvTableWidget->currentRow());
        affinityMatrix->setVerticalHeaderItem(csvTableWidget->currentRow(), new QTableWidgetItem("Actor"));
        affinityMatrix->setHorizontalHeaderItem(csvTableWidget->currentRow(), new QTableWidgetItem("Actor"));


        for( int col = 0; col < affinityMatrix->columnCount(); ++col)
        {
            if(csvTableWidget->currentRow()==col)
                affinityMatrix->setItem(csvTableWidget->currentRow(),col,
                                        new QTableWidgetItem("1"));
            else
                affinityMatrix->setItem(csvTableWidget->currentRow(),col,
                                        new QTableWidgetItem("0"));
        }

        for(int row = 0; row < affinityMatrix->rowCount() ; ++row)
        {
            if(csvTableWidget->currentRow()==row)
                affinityMatrix->setItem(row,csvTableWidget->currentRow(),
                                        new QTableWidgetItem("1"));
            else
                affinityMatrix->setItem(row,csvTableWidget->currentRow(),
                                        new QTableWidgetItem("0"));
        }

        csvTableWidget->insertRow(csvTableWidget->currentRow());
    }

}

void MainWindow::displayMenuTableView(QPoint pos)
{
    if(tableType!="Database")
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

        QAction *act = menu.exec(csvTableView->viewport()->mapToGlobal(pos));

        if (act == col)
        {
            if(csvTableView->currentIndex().column()>2)
                modeltoCSV->removeColumn(csvTableView->currentIndex().column());
            else
                statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
        }
        if (act == row)
        {
            csvAffinityModel->removeColumn(csvTableView->currentIndex().row());
            csvAffinityModel->removeRow(csvTableView->currentIndex().row());
            modeltoCSV->removeRow(csvTableView->currentIndex().row());
        }
        if (act == rename)
        {
            if(csvTableView->currentIndex().column()>2)
            {
                bool ok;
                QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                     tr("Header Name"), QLineEdit::Normal,
                                                     modeltoCSV->headerData(csvTableView->currentIndex().column(),Qt::Horizontal).toString(), &ok);

                if (ok && !text.isEmpty())
                {
                    if(csvTableView->currentIndex().column()%2!=0)
                    {
                        if(!(text.contains("Position") || text.contains("position")))
                        {
                            if(!text.contains("\n"))
                                text = text.append(" \n Position");
                            else
                                text = text.append(" Position");
                        }
                        else
                        {
                            QString header = text;
                            header.remove("Position").remove("position");
                            header.remove("\n");
                            header.append("\n Position");
                            text = header;
                        }
                        modeltoCSV->horizontalHeaderItem(csvTableView->currentIndex().column())->
                                setToolTip("The stated position, or advocacy, of the actor");
                    }
                    else
                    {
                        if(!(text.contains("Salience")|| text.contains("salience")))
                        {
                            if(!text.contains("\n"))
                                text = text.append(" \n Salience");
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
                        modeltoCSV->horizontalHeaderItem(csvTableView->currentIndex().column())->
                                setToolTip("The relative importance, or priority, for the actor");

                    }
                    modeltoCSV->setHeaderData(csvTableView->currentIndex().column(),Qt::Horizontal,text);
                    statusBar()->showMessage("Header changed");
                }
            }
            else
                statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                         " Actor, Description and Influence Columns");
        }
        if (act == posCol)
        {
            if(csvTableView->currentIndex().column()>2)
            {
                modeltoCSV->insertColumn(csvTableView->currentIndex().column());
                modeltoCSV->setHeaderData(csvTableView->currentIndex().column()-1,Qt::Horizontal,"Position");
                modeltoCSV->horizontalHeaderItem(csvTableView->currentIndex().column()-1)->
                        setToolTip("The stated position, or advocacy, of the actor");
                statusBar()->showMessage("Column Inserted, Header changed");
            }
            else
                statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                         " Actor, Description and Influence Columns");
        }
        if (act == salCol)
        {
            if(csvTableView->currentIndex().column()>2)
            {
                modeltoCSV->insertColumn(csvTableView->currentIndex().column());
                modeltoCSV->setHeaderData(csvTableView->currentIndex().column()-1,Qt::Horizontal,"Salience");
                modeltoCSV->horizontalHeaderItem(csvTableView->currentIndex().column()-1)->
                        setToolTip("The relative importance, or priority, for the actor");

                statusBar()->showMessage("Column Inserted, Header changed");
            }
            else
                statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                         " Actor, Description and Influence Columns");
        }

        if(act == newRow)
        {
            csvAffinityModel->insertColumn(csvTableView->currentIndex().row());
            csvAffinityModel->insertRow(csvTableView->currentIndex().row());

            QString actorHeader;
            actorHeader.append(" Actor ")/*.append(QString::number(rows+1))*/;
            csvAffinityModel->setHorizontalHeaderItem(csvTableView->currentIndex().row(),
                                                      new QStandardItem(actorHeader));
            csvAffinityModel->setVerticalHeaderItem(csvTableView->currentIndex().row(),
                                                    new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(csvTableView->currentIndex().row(),"CSV");
            actorHeader.clear();

            modeltoCSV->insertRow(csvTableView->currentIndex().row());
        }
    }
    else
    {
        QMenu menu(this);
        QAction *expData = menu.addAction("Export Data");

        QAction *act = menu.exec(csvTableView->viewport()->mapToGlobal(pos));

        if (act == expData)
        {
            saveTurnHistoryToCSV();
        }
    }
}

void MainWindow::actorsNameDesc(QVector <QString> actorName,QVector <QString> actorDescription)
{
    actorsName.clear();
    actorsDescription.clear();
    actorsName = actorName;
    actorsDescription = actorDescription;

    numAct= actorsName.length();
    actorsQueriedCount=numAct-1;
    if(!lineGraphActorsCheckBoxList.isEmpty())
    {
        for(int i=0; i < actorsName.length();++i)
        {
            if(lineGraphActorsCheckBoxList.at(i)->text()!=actorsName.at(i))
            {
                populateLineGraphActorsList();
                return;
            }
        }
    }
    else
        populateLineGraphActorsList();

    if(!barGraphActorsCheckBoxList.isEmpty())
    {
        for(int i=0; i < actorsName.length();++i)
        {
            if(barGraphActorsCheckBoxList.at(i)->text()!=actorsName.at(i))
            {
                populateBarGraphActorsList();
                return;
            }
        }
    }
    else
        populateBarGraphActorsList();


    if(!quadMapInitiatorsRadioButtonList.isEmpty())
    {
        for(int i=0; i < actorsName.length();++i)
        {
            if(quadMapInitiatorsRadioButtonList.at(i)->text()!=actorsName.at(i)
                    || quadMapInitiatorsRadioButtonList.length() != actorsName.length())
            {
                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
                return;
            }
            else if ((quadMapInitiatorsRadioButtonList.at(i)->text()!=actorsName.at(i)
                      || quadMapInitiatorsRadioButtonList.length() != actorsName.length())
                     && prevScenario != scenarioBox)
            {
                prevScenario=scenarioBox;
                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
                return;
            }
        }
    }
    else
        populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
}

void MainWindow::actorsInfluence(QVector<QString> actorInfluence)
{
    actorsInfl=actorInfluence;
}

void MainWindow::actorsPosition(QVector<QString> actorPosition, int dim)
{
    actorsPos[dim]=actorPosition;
}

void MainWindow::actorsSalience(QVector<QString> actorSalience,int dim)
{
    actorsSal[dim]=actorSalience;
}

void MainWindow::actAffinity(QVector<QString> actorAff, QVector<int> actorI, QVector<int> actorJ)
{
    actorAffinity=actorAff;
    actI = actorI;
    actJ = actorJ;
}

void MainWindow::scenarioModelParameters(QVector<int> modParaDB, QString seedDB)
{
    victProbModelComboBox->setCurrentIndex(modParaDB.at(0));
    pCEModelComboBox->setCurrentIndex(modParaDB.at(1));
    stateTransitionsComboBox->setCurrentIndex(modParaDB.at(2));
    votingRuleComboBox->setCurrentIndex(modParaDB.at(3));
    bigRAdjustComboBox->setCurrentIndex(modParaDB.at(4));
    bigRRangeComboBox->setCurrentIndex(modParaDB.at(5));
    thirdPartyCommitComboBox->setCurrentIndex(modParaDB.at(6));
    interVecBrgnComboBox->setCurrentIndex(modParaDB.at(7));
    bargnModelComboBox->setCurrentIndex(modParaDB.at(8));

    seedRand->setText(seedDB.trimmed());
}

void MainWindow::clearAllGraphs()
{
    lineCustomGraph->clearGraphs();
    lineGraphTitle->setText(" ");
    barGraphTitle->setText(" ");

    deleteBars();

    lineCustomGraph->replot();
    barCustomGraph->replot();

    for(int index=0; index < lineActorCBList.length();++ index)
    {
        disconnect(lineActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(lineGraphActorsCheckboxClicked(bool)));
        disconnect(barActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(barGraphActorsCheckboxClicked(bool)));
    }

    disconnect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
    disconnect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    lineGraphDimensionComboBox->clear();
    barGraphDimensionComboBox->clear();

    disconnect(lineGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(lineGraphSelectAllActorsCheckBoxClicked(bool)));
    disconnect(barGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(barGraphSelectAllActorsCheckBoxClicked(bool)));
    disconnect(barGraphBinWidthButton,SIGNAL(clicked(bool)),this, SLOT(barGraphBinWidthButtonClicked(bool)));

    removeAllScatterPoints();
}

void MainWindow :: reconnectPlotWidgetSignals()
{
    connect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
    connect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    connect(lineGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(lineGraphSelectAllActorsCheckBoxClicked(bool)));
    connect(barGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(barGraphSelectAllActorsCheckBoxClicked(bool)));
    connect(barGraphBinWidthButton,SIGNAL(clicked(bool)),this, SLOT(barGraphBinWidthButtonClicked(bool)));

}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        loadRecentFile(action->data().toString());
    }
}

void MainWindow::clearRecentFile(bool bl)
{
    recentFileSettings.remove("recentFileList");

    updateRecentFileActions();
}

void MainWindow::changeHomeDirectory(bool bl)
{
    QString dir = QFileDialog::getExistingDirectory(this,"Change Home Dir", QDir::homePath());

    if(!dir.isEmpty())
    {
        homeDirectory = dir;
        recentFileSettings.setValue("HomeDirectory",homeDirectory);
        emit homeDirChanged(homeDirectory);
        defaultDirectory= homeDirectory;
    }
}

QString MainWindow::generateTimeStamp()
{
    QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    QString timeStamp("ktab-smp-");
    timeStamp.append(QString::number(UTC.date().year())).append("-");
    timeStamp.append(QString("%1").arg(UTC.date().month(), 2, 10, QLatin1Char('0'))).append("-");
    timeStamp.append(QString("%1").arg(UTC.date().day(), 2, 10, QLatin1Char('0'))).append("__");
    timeStamp.append(QString("%1").arg(UTC.time().hour(), 2, 10, QLatin1Char('0'))).append("-");
    timeStamp.append(QString("%1").arg(UTC.time().minute(), 2, 10, QLatin1Char('0'))).append("-");
    timeStamp.append(QString("%1").arg(UTC.time().second(), 2, 10, QLatin1Char('0')));
    timeStamp.append("_GMT");

    return timeStamp;
}

void MainWindow::postgresRecentAccess()
{
    QStringList connString = connectionString.split(";");
    connectionString.clear();
    for(int i = 0 ; i < connString.length() ; ++ i)
    {
        if(i==1)
        {
            connectionString.append("Database=").append(recentFileAccess).append(";");
        }
        else
        {
            QString str = connString.at(i);
            if(!str.trimmed().isEmpty())
            {
                connectionString.append(str).append(";");
            }
        }
    }

    dbGetFilePAth(true,recentFileAccess,false);
    recentFileAccess.clear();

}

void MainWindow::removeFromRecentFileHistory(QString fileName)
{
    QMessageBox msgBox;
    msgBox.setText(QString(fileName+" not found !\n"
                           + "Do you want to remove it from the recently accessed list ?"));
    QPushButton *yesButton = msgBox.addButton(tr("Yes"), QMessageBox::ActionRole);
    QPushButton *noButton = msgBox.addButton(QMessageBox::No);

    msgBox.exec();

    if (msgBox.clickedButton() == yesButton)
    {
        QStringList files = recentFileSettings.value("recentFileList").toStringList();
        recentFileSettings.remove("recentFileList");

        int index = files.indexOf(fileName);
        files.removeAt(index);

        recentFileSettings.setValue("recentFileList",files);

        updateRecentFileActions();
    }
    else if (msgBox.clickedButton() == noButton)
    {
        msgBox.close();
    }
}

void MainWindow::connectionStrPath(QString str)
{
    connectionString = str;

    if(menuconfig == false)
    {
        runPushButtonClicked(true);
    }
    if(!recentFileAccess.isEmpty() && connectionString.contains("QPSQL"))
    {
        getPostgresDBList(connectionString,false);
    }
    else
    {
        if(!recentFileAccess.contains(".db") && !recentFileAccess.isEmpty())
        {
            displayMessage("Database","Please configure Postgres Database with Valid Credentials");
            recentFileAccess.clear();
        }
    }
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    setWindowFilePath(fileName);

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > maxRecentFilesCount)
        files.removeLast();

    settings.setValue("recentFileList", files);

    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin)
        {
            mainWin->updateRecentFileActions();
        }
    }
}

void MainWindow::updateRecentFileActions()
{
    QStringList files = recentFileSettings.value("recentFileList").toStringList();
    recentFileSettings.value("recentFileList");

    int numRecentFiles = qMin(files.size(), (int)maxRecentFilesCount);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < maxRecentFilesCount; ++j)
    {
        recentFileActs[j]->setVisible(false);
    }

    separatorAct->setVisible(numRecentFiles > 0);

}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::loadRecentFile(const QString &fileName)
{
    if(!fileName.isEmpty())
    {
        QFileInfo checkFileValid(fileName);

        if (checkFileValid.exists() && checkFileValid.isFile())
        {
            QDir dir =QFileInfo(fileName).absoluteDir();
            homeDirectory = dir.absolutePath();

            if(fileName.endsWith(".csv") || fileName.endsWith(".CSV"))
            {
                checkCSVtype(fileName);
            }
            else if(fileName.endsWith(".xml") || fileName.endsWith(".XML"))
            {
                importXmlGetFilePath(true,fileName);
            }
            else if(fileName.endsWith(".db") || fileName.endsWith(".DB"))
            {
                useHistory=false;
                if(!connectionString.contains("QSQLITE"))
                {
                    connectionString.clear();
                    connectionString.append("Driver=QSQLITE;");//connectionType
                    connectionString.append("Database=").append("None").append(";");
                }
                dbGetFilePAth(true,fileName);
            }
            else
            {
                displayMessage("Recent File", "A File and a Database Exists with same name");
            }
        }
        else if (!fileName.contains("."))
        {
            useHistory=false;
            recentFileAccess = fileName;
            if(!connectionString.contains("QPSQL"))
            {
                //POSTGRESQL
                menuconfig = true;
                configureDB(true);
            }
            else
            {
                emit getPostgresDBList(connectionString,false);
            }
        }
        else
        {
            removeFromRecentFileHistory(fileName);
        }
    }
    else
    {
        removeFromRecentFileHistory(fileName);
    }
}

void MainWindow::intializeHomeDirectory()
{
    QString homeDir = recentFileSettings.value( "HomeDirectory" ).toString();
    if(!QDir(homeDir).exists() || homeDir.isEmpty())
    {
        homeDirectory=QDir::homePath().append(QDir::separator()).append("KTAB_SMP");

        if(!QDir(homeDirectory).exists())
        {
            if(!QDir(QDir::home()).mkdir("KTAB_SMP"))
            {
                displayMessage("Home Directory", "Unable to set Home directory as "+homeDirectory);
            }
        }
    }
    else if(!homeDir.isEmpty())
    {
        homeDirectory = homeDir;
    }

    recentFileSettings.setValue("HomeDirectory",homeDirectory);

    defaultDirectory= homeDirectory;
}

void MainWindow::checkCSVtype(QString fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly) && file.size() > 0)
    {
        QTextStream in(&file);                 // read to text stream

        if (!in.atEnd())
        {
            // read one line from textstream(separated by "\n")
            QString fileLine = in.readLine();

            // parse the read line into separate pieces(tokens) with "," as the delimiter
            QStringList lineToken = fileLine.split(",", QString::SkipEmptyParts);
            if(lineToken.length()>2)
            {
                file.close();
                csvGetFilePAth(true,fileName);
            }
            else if(lineToken.length()==2)
            {
                file.close();
                if(tableType=="Database")
                {
                    setCurrentFile(fileName);
                    emit importColors(fileName,actorsName.length());
                }
                else
                {
                    displayMessage("Actors color picker","Please Import DB or RUN SMP Model");
                }
            }
            else
            {
                file.close();
            }

        }

    }
    updateRecentFileActions();

}

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
