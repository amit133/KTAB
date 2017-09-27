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

#include "smp.h"
#include "demosmp.h"
#include <functional>
#include "mainwindow.h"

// Copied this code snippet from demosmp.cpp

using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;
using KBase::VPModel;
using KBase::KException;

namespace DemoSMP {

using std::function;
using std::get;
using std::string;
using std::vector;

using KBase::ReportingLevel;

using SMPLib::SMPModel;
using SMPLib::SMPActor;
using SMPLib::SMPState;

// -------------------------------------------------
// this binds the given parameters and returns the λ-fn necessary to stop the SMP appropriately
function<bool(unsigned int, const State *)>
smpStopFn(unsigned int minIter, unsigned int maxIter, double minDeltaRatio, double minSigDelta) {
    auto  sfn = [minIter, maxIter, minDeltaRatio, minSigDelta](unsigned int iter, const State * s) {
        bool tooLong = (maxIter <= iter);
        bool longEnough = (minIter <= iter);
        bool quiet = false;
        auto sf = [](unsigned int i1, unsigned int i2, double d12) {
            return KBase::getFormattedString("sDist [%2i,%2i] = %.2E   ", i1, i2, d12);;
        };
        auto s0 = ((const SMPState*)(s->model->history[0]));
        auto s1 = ((const SMPState*)(s->model->history[1]));
        auto d01 = SMPModel::stateDist(s0, s1) + minSigDelta;
        string logMsg;
        logMsg += sf(0, 1, d01);
        auto sx = ((const SMPState*)(s->model->history[iter - 0]));
        auto sy = ((const SMPState*)(s->model->history[iter - 1]));
        auto dxy = SMPModel::stateDist(sx, sy);
        logMsg += sf(iter - 1, iter - 0, dxy);
        LOG(INFO) << logMsg;
        const double aRatio = dxy / d01;
        quiet = (aRatio < minDeltaRatio);
        LOG(INFO) << KBase::getFormattedString(
                         "Fractional change compared to first step: %.4f  (target=%.4f)\n",
                         aRatio, minDeltaRatio);
        return tooLong || (longEnough && quiet);
    };
    return sfn;
};


} // end of namespace


void MainWindow::configureDB(bool bl)
{
    quint8 conTypeIndex = 0 ;// 0- SQLITE, 1 - PostgreSQL index
    if(connectionString.contains("QPSQL"))
    {
        conTypeIndex = 1;
    }
    dbDialog = new DatabaseDialog;
    //    connect(dbDialog,SIGNAL(configDbDriver(QString)),dbObj,SLOT(addDatabase(QString)));
    connect(dbDialog,SIGNAL(connectionStringPath(QString,QStringList)),this,SLOT(connectionStrPath(QString,QStringList)));
    dbDialog->showDialog(conTypeIndex,dbParaMemory);
}

void MainWindow::postgresDBList(QStringList *dbList, bool imp)
{
    if(recentFileAccess.isEmpty()==true)
    {
        QDialog * dbListDialog = new QDialog(this);
        QGridLayout * mainLayout = new QGridLayout;
        dbListDialog->setLayout(mainLayout);
        dbListDialog->setMaximumWidth(500);
        dbListDialog->setMaximumHeight(300);
        dbListDialog->setModal(true);
        dbListDialog->setWindowTitle("KTAB SMP: Databases ");
        dbListDialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QLabel *actorHLabel= new QLabel("Existing Postgres DB List");
        mainLayout->addWidget(actorHLabel,0,0,Qt::AlignHCenter);

        QListView * dbListView = new QListView(this);
        QStandardItemModel *dbListModel = new QStandardItemModel;

        for (int i = 0 ; i < dbList->count(); ++i)
        {
            QStandardItem * item = new QStandardItem(dbList->at(i));
            item->setEditable(false);
            dbListModel->setItem(i,0,item);
        }
        dbListView->setModel(dbListModel);

        connect(dbListView,SIGNAL(clicked(QModelIndex)),this,SLOT(dbListClicked(QModelIndex)));

        mainLayout->addWidget(actorHLabel,1,0,Qt::AlignHCenter);
        mainLayout->addWidget(dbListView,2,0);

        dbDialogName = new QLineEdit(generateTimeStamp());
        QPushButton * generateTS = new QPushButton;
        generateTS->setIcon(QIcon("://images/regenerate.png"));
        generateTS->setMaximumWidth(50);
        connect(generateTS,SIGNAL(clicked(bool)),this,SLOT(getTimeStamp(bool)));

        QGridLayout * fileNameLayout = new QGridLayout;
        fileNameLayout->addWidget(dbDialogName,0,0);
        fileNameLayout->addWidget(generateTS,0,3);

        mainLayout->addLayout(fileNameLayout,3,0);

        QGridLayout * controlsLayout = new QGridLayout;
        QPushButton * donePB = new QPushButton("Done");
        QPushButton * cancelPB = new QPushButton("Cancel");
        controlsLayout->addWidget(donePB,0,Qt::AlignRight);
        controlsLayout->addWidget(cancelPB,0,Qt::AlignLeft);

        connect(donePB,SIGNAL(clicked(bool)),dbListDialog,SLOT(close()));
        connect(donePB,SIGNAL(clicked(bool)),this,SLOT(dbDonePushButtonClicked(bool)));
        connect(cancelPB,SIGNAL(clicked(bool)),dbListDialog,SLOT(close()));

        mainLayout->addLayout(controlsLayout,4,0,Qt::AlignBottom);

        //Different options for Import and Run
        if(true==imp)//DB import
        {
            dbDialogName->setText(dbList->last());
            generateTS->hide();
        }
        else
        {
            generateTS->show();
        }
        dbListDialog->show();
    }
    else
    {
        if(dbList->contains(recentFileAccess))
        {
            postgresRecentAccess();
        }
        else
        {
            removeFromRecentFileHistory(recentFileAccess);
            recentFileAccess.clear();
        }
    }
}

void MainWindow::getTimeStamp(bool bl)
{
    dbDialogName->setText(generateTimeStamp());
}

void MainWindow::configUsingMenu(bool bl)
{
    menuconfig = true;
}

void MainWindow::dbListClicked(QModelIndex indx)
{
    dbDialogName->setText(indx.data().toString());
}

void MainWindow::dbDonePushButtonClicked(bool bl)
{
    //index 1
    QString dbName = dbDialogName->text();
    dbPath=dbName;
    QStringList connString = connectionString.split(";");

    connectionString.clear();
    for(int i = 0 ; i < connString.length() ; ++ i)
    {
        if(i==1)
        {
            connectionString.append("Database=").append(dbName).append(";");
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

    if(importedDBFile==false)
    {
        runModel(connectionString,dbName);
    }
    else
    {
        dbGetFilePAth(true,dbPath,false);
    }
}

void MainWindow::runPushButtonClicked(bool bl)
{
    Q_UNUSED(bl)
    importedDBFile=false;
    if(connectionString.isEmpty())
    {
        menuconfig = false;
        configureDB(true);
    }
    else
    {
        if(connectionString.contains("QSQLITE")) // for SQLite
        {
            QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
            QString name (UTC.toString());

            name.replace(" ","_").replace(":","_");

            QFileDialog fileDialog(this);
            QString dbFilePath = fileDialog.getSaveFileName(this, tr("Save DB file as "),
                                                            QString(homeDirectory+QDir::separator()+name),tr("DB File (*.db)"),
                                                            0,QFileDialog::DontConfirmOverwrite);
            if(!dbFilePath.isEmpty())
            {
                QString connectionStr;
                connectionStr.append("Driver=QSQLITE;");//connectionType
                //            connectionStr.append("Server=localhost;");//host address
                connectionStr.append("Database=").append(dbFilePath.remove(".db").trimmed()).append(";");

                QDir dir = QFileInfo(dbFilePath.append(".db")).absoluteDir();
                homeDirectory = dir.absolutePath();

                connectionString = connectionStr;
                runModel(connectionString,dbFilePath.remove(".db"));
            }
        }
        else // Postgres
        {
            emit getPostgresDBList(connectionString,false);// true == imported, false == run
        }
    }
}

void MainWindow::runModel(QString conStr, QString fileName)
{

    QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    QString name("ktab-smp-");
    name.append(QString::number(UTC.date().year())).append("-");
    name.append(QString("%1").arg(UTC.date().month(), 2, 10, QLatin1Char('0'))).append("-");
    name.append(QString("%1").arg(UTC.date().day(), 2, 10, QLatin1Char('0'))).append("__");
    name.append(QString("%1").arg(UTC.time().hour(), 2, 10, QLatin1Char('0'))).append("-");
    name.append(QString("%1").arg(UTC.time().minute(), 2, 10, QLatin1Char('0'))).append("-");
    name.append(QString("%1").arg(UTC.time().second(), 2, 10, QLatin1Char('0')));
    name.append("_GMT");

    logSMPDataOptionsAnalysis();

    QString connectDBString;

    connectDBString = conStr;
    //    qDebug()<<connectDBString << "db";

    if(!connectDBString.isEmpty())
    {
        runButton->setEnabled(false);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");

        statusBar()->showMessage(" Please wait !! Executing SMP ....  This may take a while ....");

        QApplication::setOverrideCursor(QCursor(QPixmap("://images/hourglass.png")));
        QApplication::processEvents();

        //get parameters from GUI
        getParametersValues();

        // A code snippet from Demosmp.cpp to run the model

        using KBase::dSeed;
        auto sTime = KBase::displayProgramStart(DemoSMP::appName, DemoSMP::appVersion);
        uint64_t seed = dSeed;

        //Collect flags and Data from GUI
        if(seedRand->text().length()>0 && !seedRand->text().isNull())
        {
            seed = seedRand->text().toULongLong();

        }
        if (0 == seed)
        {
            PRNG * rng = new PRNG();
            seed = rng->setSeed(seed); // 0 == get a random number
            delete rng;
            rng = nullptr;
            seedRand->setText(QString::number(seed));
            statusBar()->showMessage("Random Seed generated for SMP Model Run "+QString::number(seed));
        }
        if(seed==dSeed)
        {
            seedRand->setText(QString::number(seed));
            statusBar()->showMessage("Default Seed Used for SMP Model Run "+QString::number(seed));
        }

        // JAH 20160730 vector of SQL logging flags for 5 groups of tables:
        // 0 = Information Tables, 1 = Position Tables, 2 = Challenge Tables,
        // 3 = Bargain Resolution Tables, 4 = VectorPosition table
        // JAH 20161010 added group 4 for only VectorPos so it can be logged alone
        std::vector<bool> sqlFlags = {true,true,true,true,true};

        //log minimum
        if(logMinimum->isChecked())
        {
            sqlFlags = {true,false,false,false,true};
        }

        // Notice that we NEVER use anything but the default seed.
        //        printf("Using PRNG seed:  %020llu \n", seed);
        //        printf("Same seed in hex:   0x%016llX \n", seed);

        QString inputDataFile;
        if(savedAsXml==true) {
            inputDataFile = xmlPath;
        } else {
            inputDataFile = csvPath;
        }
        bool scenarioIdAvailable = false;

        bool isDbConnected = SMPLib::SMPModel::loginCredentials(connectDBString.toStdString());

        if(false==isDbConnected)
        {
         QApplication::restoreOverrideCursor();
         displayMessage("Exception",QString::fromStdString(KBase::Model::getLastError()));
        }
        else
        {
             currentScenarioId = QString::fromStdString(SMPLib::SMPModel::runModel
                                                       (sqlFlags,
                                                        inputDataFile.toStdString(),seed,false,parameters));
            if(true==currentScenarioId.isEmpty())
            {
                QApplication::restoreOverrideCursor();
                displayMessage("Exception",QString::fromStdString(KBase::Model::getLastError()));
                scenarioIdAvailable=false;
            }
            else
            {
                scenarioIdAvailable=true;
            }
        }

        if(true == isDbConnected && true == scenarioIdAvailable)
        {
            KBase::displayProgramEnd(sTime);

            QApplication::restoreOverrideCursor();
            statusBar()->showMessage(" Process Completed !! ");

            smpDBPath(fileName);
        }
        else
        {
            QApplication::restoreOverrideCursor();

            statusBar()->showMessage("SMP Model Run Cancelled because of an Exception !! ");
            QMessageBox::warning(this,"Exception", "SMP Model Run Cancelled because of an Exception!! ",QMessageBox::Ok);
            runButton->setEnabled(true);
            runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");
        }
    }
    else
    {
        statusBar()->showMessage("SMP Model Run Cancelled !! ");
        QMessageBox::warning(this,"Warning", "SMP Model Run Cancelled !! ",QMessageBox::Ok);
        runButton->setEnabled(true);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: green;");

    }
}

void MainWindow::smpDBPath(QString smpdbPath)
{
    useHistory=true;
    sankeyOutputHistory=false;
    dbGetFilePAth(true,smpdbPath,true);
}

void MainWindow::disableRunButton(QTableWidgetItem *itm)
{
    //    disconnect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));

    if(itm->column()==0)
    {
        affinityMatrix->setVerticalHeaderItem(itm->row(),new QTableWidgetItem(itm->text()));
        affinityMatrix->setHorizontalHeaderItem(itm->row(),new QTableWidgetItem(itm->text()));
        runButton->setEnabled(false);
        runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");

    }
}

void MainWindow::logSMPDataOptionsAnalysis()
{
    QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    QString name("ktab-smp-");
    name.append(QString::number(UTC.date().year())).append("-");
    name.append(QString("%1").arg(UTC.date().month(), 2, 10, QLatin1Char('0'))).append("-");
    name.append(QString("%1").arg(UTC.date().day(), 2, 10, QLatin1Char('0'))).append("__");
    name.append(QString("%1").arg(UTC.time().hour(), 2, 10, QLatin1Char('0'))).append("-");
    name.append(QString("%1").arg(UTC.time().minute(), 2, 10, QLatin1Char('0'))).append("-");
    name.append(QString("%1").arg(UTC.time().second(), 2, 10, QLatin1Char('0')));
    name.append("_GMT");

    if(logDefaultAct->isChecked()==true)
    {
        if(logFileName.isEmpty())
        {
            logFileName = name.append("_log.txt");
        }

        loggerConf.setGlobally(el::ConfigurationType::Filename, logFileName.toStdString());
        // Enable all the logging
        loggerConf.set(el::Level::Global, el::ConfigurationType::Enabled, "true");
        el::Loggers::reconfigureAllLoggers(loggerConf);
    }
    else if(logNewAct->isChecked()==true)
    {
        fclose(stdout);
        fp_old = *stdout;
        QString logFileNewName = QString(homeDirectory+QDir::separator()+name);
        QString saveLogFilePath = QFileDialog::getSaveFileName(this, tr("Save Log File to "),logFileNewName,
                                                               tr("Text File (*.txt)"),0,QFileDialog::DontConfirmOverwrite);
        if(!saveLogFilePath.isEmpty())
        {
            if(saveLogFilePath.endsWith(".txt"))
            {
                saveLogFilePath.remove(".txt");
            }
            QDir dir =QFileInfo(saveLogFilePath).absoluteDir();
            homeDirectory = dir.absolutePath();

            logFileNewName=saveLogFilePath;

            std::string logFileName = logFileNewName.append("_log.txt").toStdString();
            loggerConf.setGlobally(el::ConfigurationType::Filename, logFileName);
            // Enable all the logging
            loggerConf.set(el::Level::Global, el::ConfigurationType::Enabled, "true");
            el::Loggers::reconfigureAllLoggers(loggerConf);
        }
    }
    else
    {
        if(logNoneAct->isChecked()==true)
        {
            // Disable all the logging
            loggerConf.set(el::Level::Global, el::ConfigurationType::Enabled, "false");

            el::Loggers::reconfigureAllLoggers(loggerConf);
        }
    }

}

//Model Parameters

void MainWindow::initializeModelParametersDock()
{
    modelParametersFrame = new QFrame(modelParametersDock);
    VLayout  = new QGridLayout(modelParametersFrame);

    frames = new QFrame(modelParametersDock);
    frames->setFrameShape(QFrame::StyledPanel);//change to flat if required
    VLayout->addWidget(frames,0,0);

    //LogMinimum
    logMinimum = new QCheckBox("Minimum Logging");
    logMinimum->setChecked(true);
    logMinimum->setToolTip("Checking this disables logging except for: model / data information,"
                           "\nactor positions, and position history ");
    VLayout->addWidget(logMinimum,1,0);

    //Seed
    //    QLabel * seedLabel = new QLabel("Randomizer Seed");
    // VLayout->addWidget(seedLabel,2,0);
    seedRand = new QLineEdit;
    seedRand->setPlaceholderText("Enter Random Seed");
    seedRand->setToolTip("Set the initial seed for the random number generator;"
                         "\nif 0 is entered, the seed will be generated randomly; "
                         "\nthe model will use a default seed if this is left empty");
    VLayout->addWidget(seedRand,2,0);

    //RUN Button
    runButton = new QPushButton;
    runButton->setText("RUN");
    runButton->setEnabled(false);
    runButton->setStyleSheet("border-style: outset; border-width: 2px;border-color: red;");

    runButton->setToolTip("Run the model");
    VLayout->addWidget(runButton,3,0);

    connect(runButton,SIGNAL(clicked(bool)),this,SLOT(runPushButtonClicked(bool)));

    //Radio buttons //Section1
    QGridLayout *parametersGLayout = new QGridLayout(frames);

    victProbModelComboBox = new QComboBox;
    pCEModelComboBox = new QComboBox;
    stateTransitionsComboBox = new QComboBox;
    votingRuleComboBox = new QComboBox;
    bigRAdjustComboBox = new QComboBox;
    bigRRangeComboBox = new QComboBox;
    thirdPartyCommitComboBox = new QComboBox;
    interVecBrgnComboBox = new QComboBox;
    bargnModelComboBox = new QComboBox;

    QLabel *victProbLabel = new QLabel(" VictoryProbModel ");
    victProbLabel->setToolTip("Controls the rate at which the probability of a coalition supporting "
                              "\nan option winning against the coalition proposing it increases as the "
                              "\nstrength ratios increase");
    victProbLabel->setFrameStyle(QFrame::StyledPanel);
    victProbLabel->setAlignment(Qt::AlignHCenter);

    QLabel *pCELabel =  new QLabel(" ProbCondorcetElection ");
    pCELabel->setToolTip("Controls the type of probabilistic condorcet election used");
    pCELabel->setFrameStyle(QFrame::StyledPanel);
    pCELabel->setAlignment(Qt::AlignHCenter);

    QLabel *stateTransitionsLabel =  new QLabel(" StateTransition ");
    stateTransitionsLabel->setToolTip("Controls how the winning bargain in an actor's\n queue is chosen among all bargains");
    stateTransitionsLabel->setFrameStyle(QFrame::StyledPanel);
    stateTransitionsLabel->setAlignment(Qt::AlignHCenter);

    QLabel *votingRuleLabel =  new QLabel(" VotingRule ");
    votingRuleLabel->setToolTip("Controls how the amount of influence an actor will exert between two options "
                                "\ndepends on the perceived difference in utilities");
    votingRuleLabel->setFrameStyle(QFrame::StyledPanel);
    votingRuleLabel->setAlignment(Qt::AlignHCenter);

    QLabel *bigRAdjustLabel =  new QLabel(" BigRAdjust ");
    bigRAdjustLabel->setToolTip("Controls how accurately actor i is able to estimate, relative to an anchor\nof its own risk attitude,"
                                " the risk attitude\n of actor j (which is known to the model)");
    bigRAdjustLabel->setFrameStyle(QFrame::StyledPanel);
    bigRAdjustLabel->setAlignment(Qt::AlignHCenter);

    QLabel *bigRRangeLabel =  new QLabel(" BigRRange ");
    bigRRangeLabel->setToolTip("Controls actors' risk tolerances, and hence the\n curvature of their utility functions");
    bigRRangeLabel->setFrameStyle(QFrame::StyledPanel);
    bigRRangeLabel->setAlignment(Qt::AlignHCenter);

    QLabel *thirdPartyCommitLabel =  new QLabel(" ThirdPartyCommit ");
    thirdPartyCommitLabel->setToolTip("Controls how committed a third party actor k is in a challenge between "
                                      "actors i and j");
    thirdPartyCommitLabel->setFrameStyle(QFrame::StyledPanel);
    thirdPartyCommitLabel->setAlignment(Qt::AlignHCenter);

    QLabel *interVecBrgnLabel =  new QLabel(" InterVecBrgn ");
    interVecBrgnLabel->setToolTip("Controls how proposed positions are interpolated between the\n positions of actor i and"
                                  " j in a bargain");
    interVecBrgnLabel->setFrameStyle(QFrame::StyledPanel);
    interVecBrgnLabel->setAlignment(Qt::AlignHCenter);

    QLabel *bargnModelLabel =  new QLabel(" BargnModel ");
    bargnModelLabel->setToolTip("Controls from which actor's perspective the probability of\n success is used to "
                                "interpolate bargains");
    bargnModelLabel->setFrameStyle(QFrame::StyledPanel);
    bargnModelLabel->setAlignment(Qt::AlignHCenter);

    QStringList victList;
    victList << "Linear" << "Square" << "Quartic" << "Octic" << "Binary";

    QStringList pceList;
    pceList << "Conditional" << "MarkovIncentive" << "MarkovUniform";

    QStringList stateList;
    stateList << "Deterministic" << "Stochastic";

    QStringList votingList;
    votingList << "Binary" << "PropBin"<< "Prop"<< "PropCbc"<< "Cubic"<< "ASymProsp";

    QStringList bigAdjList;
    bigAdjList << "None" << "OneThird" << "Half"<< "TwoThirds"<< "Full";

    QStringList bigRangeList;
    bigRangeList << "Min" << "Mid" << "Max";

    QStringList thridpartyList;
    thridpartyList << "NoCommit" << "SemiCommit" << "FullCommit";

    QStringList interVecList;
    interVecList << "S1P1" << "S2P2" << "S2PMax";

    QStringList bargnList;
    bargnList << "InitOnlyInterp" << "InitRcvrInterp" << "PWCompInterp";

    QVBoxLayout * v1 = new QVBoxLayout;
    victProbModelComboBox->addItems(victList);
    victProbModelComboBox->setToolTip("Controls the rate at which the probability of a coalition supporting "
                                      "\nan option winning against the coalition proposing it increases as the "
                                      "\nstrength ratios increase");
    victProbModelComboBox->setItemData(0,"A 2:1 ratio gives a probability of 2/3 to the "
                                         "\nstronger coalition (default)", Qt::ToolTipRole);
    victProbModelComboBox->setItemData(1,"A 2:1 ratio gives a probability of 4/5 to the stronger coalition",
                                       Qt::ToolTipRole);
    victProbModelComboBox->setItemData(2,"A 2:1 ratio gives a probability of 16/17 to the stronger coalition",
                                       Qt::ToolTipRole);
    victProbModelComboBox->setItemData(3,"A 2:1 ratio gives a probability of 256/257 to the stronger coalition",
                                       Qt::ToolTipRole);
    victProbModelComboBox->setItemData(4,"Any significant percentage difference gives a probability of 1 to "
                                         "\nthe stronger coalition", Qt::ToolTipRole);
    v1->addWidget(victProbLabel,0,Qt::AlignBottom);
    v1->addWidget(victProbModelComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v1,0,0);

    QVBoxLayout * v2 = new QVBoxLayout;
    pCEModelComboBox->addItems(pceList);
    pCEModelComboBox->setToolTip("Controls the type of probabilistic condorcet election used");
    pCEModelComboBox->setItemData(0,"PCE uses single-step conditional probabilities (default)", Qt::ToolTipRole);
    pCEModelComboBox->setItemData(1,"PCE uses a Markov process in which challenge probabilities are\n proportional"
                                    " to promoting influence",Qt::ToolTipRole);
    pCEModelComboBox->setItemData(2,"PCE uses a Markov process in which challenge \nprobabilities are "
                                    "uniform",Qt::ToolTipRole);
    v2->addWidget(pCELabel,0,Qt::AlignBottom);
    v2->addWidget(pCEModelComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v2,0,1);

    QVBoxLayout * v3 = new QVBoxLayout;
    stateTransitionsComboBox->addItems(stateList);
    stateTransitionsComboBox->setToolTip("Controls how the winning bargain in an actor's queue is chosen among all bargains");
    stateTransitionsComboBox->setItemData(0,"The bargain which has the strongest coalition, and hence the highest"
                                            "\n probability of winning, wins (default)", Qt::ToolTipRole);
    stateTransitionsComboBox->setItemData(1,"The probability of winning for each bargain is proportional to its "
                                            "\nrelative coalition strength",Qt::ToolTipRole);

    v3->addWidget(stateTransitionsLabel,0,Qt::AlignBottom);
    v3->addWidget(stateTransitionsComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v3,0,2);

    QVBoxLayout * v4 = new QVBoxLayout;
    votingRuleComboBox->addItems(votingList);
    votingRuleComboBox->setToolTip("Controls how the amount of influence an actor will exert between two options "
                                   "\ndepends on the perceived difference in utilities");
    votingRuleComboBox->setItemData(2,"The vote is linearly proportional to the difference in utilities (default)",
                                    Qt::ToolTipRole);
    votingRuleComboBox->setItemData(1,"The vote is proportional to the weighted average of Prop (80%) and Binary (20%)",
                                    Qt::ToolTipRole);
    votingRuleComboBox->setItemData(0,"The actor exerts all influence, regardless of the difference in utilities",
                                    Qt::ToolTipRole);
    votingRuleComboBox->setItemData(3,"The vote is proportional to the average of Prop and Cubic",
                                    Qt::ToolTipRole);
    votingRuleComboBox->setItemData(5,"Influence is exerted asymmetrically: It is proportional to the difference"
                                      "\nof utilities if negative (a loss in utility)."
                                      "\nIt is proportional to 2/3 of the difference, if positive (a gain in utility).",
                                    Qt::ToolTipRole);
    votingRuleComboBox->setItemData(4,"The vote is proportional to the cubed difference in utilities", Qt::ToolTipRole);
    v4->addWidget(votingRuleLabel,0,Qt::AlignBottom);
    v4->addWidget(votingRuleComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v4,1,0);

    QVBoxLayout * v5 = new QVBoxLayout;
    bigRAdjustComboBox->addItems(bigAdjList);
    bigRAdjustComboBox->setToolTip("Controls how accurately actor i is able to estimate, relative to an anchor of its own risk attitude,"
                                   "\nthe risk attitude of actor j (which is known to the model)");
    bigRAdjustComboBox->setItemData(0,"Actor i judges actor j's risk attitude as being identical to its risk attitude",
                                    Qt::ToolTipRole);
    bigRAdjustComboBox->setItemData(1,"Actor i estimates actor j's risk attitude by interpolating between them, such that its estimate"
                                      "\nis closer (2/3 anchored, 1/3 adjusted) to its risk attitude (default)",
                                    Qt::ToolTipRole);
    bigRAdjustComboBox->setItemData(2,"Actor i estimates actor j's risk attitude by interpolating midway between its "
                                      "\nrisk attitude and actor j's actual risk attitude",
                                    Qt::ToolTipRole);
    bigRAdjustComboBox->setItemData(4,"Actor i judges actor j's risk attitude correctly",
                                    Qt::ToolTipRole);
    bigRAdjustComboBox->setItemData(3,"Actor i estimates actor j's risk attitude by interpolating between them, such that "
                                      "\nits estimate is closer (1/3 anchored, 2/3 adjusted) to actor j's risk attitude",
                                    Qt::ToolTipRole);

    v5->addWidget(bigRAdjustLabel,0,Qt::AlignBottom);
    v5->addWidget(bigRAdjustComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v5,1,1);

    QVBoxLayout * v6 = new QVBoxLayout;
    bigRRangeComboBox->addItems(bigRangeList);
    bigRRangeComboBox->setToolTip("Controls actors' risk tolerances, and hence the curvature of their utility functions");
    bigRRangeComboBox->setItemData(0,"Sets risk tolerances in the range [0,1] such that actors with the most probable"
                                     "\nposition are perfectly risk averse (1), while actors holding the least probable "
                                     "\nposition are perfectly risk tolerant (0)",
                                   Qt::ToolTipRole);
    bigRRangeComboBox->setItemData(1,"Sets risk tolerances in the range [-½,1] such that actors with the most probable "
                                     "\nposition are perfectly risk averse (1), while actors holding the least probable"
                                     "\nposition are somewhat risk seeking, with an aversion of -½ (default)If the coalition "
                                     "\nactor k has joined loses, k must take the position of the winning coalition; otherwise"
                                     "\nit does not need to change position (default)",
                                   Qt::ToolTipRole);
    bigRRangeComboBox->setItemData(2,"Sets risk tolerances in the range [-1,1] such that actors with the most probable position"
                                     "\nare perfectly risk averse (1), while actors holding the least probable position are "
                                     "\nperfectly risk seeking (-1)",
                                   Qt::ToolTipRole);
    v6->addWidget(bigRRangeLabel,0,Qt::AlignBottom);
    v6->addWidget(bigRRangeComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v6,1,2);

    QVBoxLayout * v7 = new QVBoxLayout;
    thirdPartyCommitComboBox->addItems(thridpartyList);
    thirdPartyCommitComboBox->setToolTip("Controls how committed a third party actor k is in a challenge between "
                                         "actors i and j");
    thirdPartyCommitComboBox->setItemData(0,"No matter which coalition actor k joins (i or j), actor k never changes position",
                                          Qt::ToolTipRole);
    thirdPartyCommitComboBox->setItemData(1,"If the coalition joined by actor k loses, k must take the position of the winning"
                                            "\ncoalition; otherwise it does not need to change position (default)",
                                          Qt::ToolTipRole);
    thirdPartyCommitComboBox->setItemData(2,"Actor k is fully committed to the coalition it joins, and must adopt the position "
                                            "\nof the winning coalition",
                                          Qt::ToolTipRole);
    v7->addWidget(thirdPartyCommitLabel,0,Qt::AlignBottom);
    v7->addWidget(thirdPartyCommitComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v7,2,0);

    QVBoxLayout * v8 = new QVBoxLayout;
    interVecBrgnComboBox->addItems(interVecList);
    interVecBrgnComboBox->setToolTip("Controls how proposed positions are interpolated between the positions of actor i and"
                                     " j in a bargain");
    interVecBrgnComboBox->setItemData(0,"Proposed postions for each actor are computed as a weighted average of their "
                                        "\ncurrent positions, where the weights are the products of salience and probability of success",
                                      Qt::ToolTipRole);
    interVecBrgnComboBox->setItemData(1,"Proposed postions for each actor are computed as a weighted average of their current"
                                        "\npositions, where the weights are the squared products of salience*probability of "
                                        "\nsuccess (default)",
                                      Qt::ToolTipRole);
    interVecBrgnComboBox->setItemData(2,"Proposed postions for each actor are computed as asymmetric shifts from their current"
                                        "\npositions, which is a function of squared salience and truncated difference in "
                                        "\nprobability of success",
                                      Qt::ToolTipRole);
    v8->addWidget(interVecBrgnLabel,0,Qt::AlignBottom);
    v8->addWidget(interVecBrgnComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v8,2,1);

    QVBoxLayout * v9 = new QVBoxLayout;
    bargnModelComboBox->addItems(bargnList);
    bargnModelComboBox->setToolTip("Controls from which actor's perspective the probability of success is\n used to "
                                   "interpolate bargains");
    bargnModelComboBox->setItemData(0,"Bargains are only computed from the initiating actor's perspective (default)",
                                    Qt::ToolTipRole);
    bargnModelComboBox->setItemData(1,"Bargains are computed from the perspective of both the initiating actor and receiving actor",
                                    Qt::ToolTipRole);
    bargnModelComboBox->setItemData(2,"Bargains are computed as an effective power-weighted average of both actor's perspectives",
                                    Qt::ToolTipRole);
    v9->addWidget(bargnModelLabel,0,Qt::AlignBottom);
    v9->addWidget(bargnModelComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v9,2,2);

    modelParametersDock->setWidget(modelParametersFrame);
    addDockWidget(Qt::LeftDockWidgetArea, modelParametersDock);
    viewMenu->addAction(modelParametersDock->toggleViewAction());
    viewMenu->actions().at(0)->setToolTip("Show/hide the panel of SMP model parameters");


    defParameters = QVector<int>::fromStdVector(SMPLib::SMPModel::getDefaultModelParameters());
    setDefaultParameters();

    modelParametersDock->setVisible(true);
}

void MainWindow::getParametersValues()
{
    victProbModel= victProbModelComboBox->currentIndex();
    pCEModel = pCEModelComboBox->currentIndex();
    stateTransitions = stateTransitionsComboBox->currentIndex();
    votingRule = votingRuleComboBox->currentIndex();
    bigRAdjust = bigRAdjustComboBox->currentIndex();
    bigRRange = bigRRangeComboBox->currentIndex();
    thirdPartyCommit= thirdPartyCommitComboBox->currentIndex();
    interVecBrgn = interVecBrgnComboBox->currentIndex();
    bargnModel = bargnModelComboBox->currentIndex();

    parameters={victProbModel,pCEModel,stateTransitions,votingRule,bigRAdjust,bigRRange,thirdPartyCommit,
                interVecBrgn,bargnModel};

}

void MainWindow::setDefaultParameters()
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

}

void MainWindow::saveTurnHistoryToCSV()
{

    QString csvFileNameLocation = QFileDialog::getSaveFileName(
                this, tr("Save Log File to "),QString(homeDirectory+QDir::separator()+"Log"),"CSV File (*.csv)");

    if(!csvFileNameLocation.isEmpty())
    {
        QDir dir =QFileInfo(csvFileNameLocation).absoluteDir();
        homeDirectory = dir.absolutePath();
        //        setCurrentFile(csvFileNameLocation);
        if(sankeyOutputHistory==true)
        {
            bool isDbConnected = SMPLib::SMPModel::loginCredentials(connectionString.toStdString());

            if(false==isDbConnected)
            {
                displayMessage("Exception",QString::fromStdString(KBase::Model::getLastError()));
            }
            else
            {
                QString exceptionMsg;
                try
                {
                    SMPLib::md0->sankeyOutput(csvFileNameLocation.toStdString()
                                              ,dbPath.toStdString(),scenarioBox.toStdString());
                }
                catch (KException &ke)
                {
                    exceptionMsg = QString::fromStdString(ke.msg);
                    displayMessage("Exception",exceptionMsg);
                    LOG(INFO) << exceptionMsg.toStdString();
                }
                catch (std::exception &std_ex)
                {
                    exceptionMsg = std_ex.what();
                    displayMessage("Exception",exceptionMsg);
                    LOG(INFO) << exceptionMsg.toStdString();
                }
                catch (...)
                {
                    exceptionMsg = "SMPLib::SMPModel::getQuadMapPoint: Unknown Exception Caught while getting QuadMap values";
                    displayMessage("Exception",exceptionMsg);
                    LOG(INFO) << exceptionMsg.toStdString();
                }
            }


            //                SMPLib::md0->sankeyOutput(csvFileNameLocation.toStdString()
            //                                          ,dbPath.toStdString(),scenarioBox.toStdString());
            statusBar()->showMessage("Turn History is stored in : " +
                                     csvFileNameLocation+ "_effPow.csv and " + " " +
                                     csvFileNameLocation+ "_posLog.csv files",2000);
        }
    }
    else
    {

        QString exceptionMsg;
        try
        {
            SMPLib::md0->sankeyOutput(csvFileNameLocation.toStdString());
        }
        catch (KException &ke)
        {
            exceptionMsg = QString::fromStdString(ke.msg);
            displayMessage("Exception",exceptionMsg);
            LOG(INFO) << exceptionMsg.toStdString();
        }
        catch (std::exception &std_ex)
        {
            exceptionMsg = std_ex.what();
            displayMessage("Exception",exceptionMsg);
            LOG(INFO) << exceptionMsg.toStdString();
        }
        catch (...)
        {
            exceptionMsg = "SMPLib::SMPModel::getQuadMapPoint: Unknown Exception Caught while getting QuadMap values";
            displayMessage("Exception",exceptionMsg);
            LOG(INFO) << exceptionMsg.toStdString();
        }
    }
    //    SMPLib::md0->sankeyOutput(csvFileNameLocation.toStdString());
    statusBar()->showMessage("Turn History is stored in : " +
                             csvFileNameLocation+ "_effPow.csv and " + " " +
                             csvFileNameLocation+ "_posLog.csv files",2000);
}




// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
