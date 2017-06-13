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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "csv.h"
#include "database.h"
#include "xmlparser.h"
#include "colorpickerdialog.h"
#include "popupwidget.h"
#include "databasedialog.h"
#include "../qcustomplot/qcustomplot.h"

#include <QtSql>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QRegExp>

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <QDateTime>

#include <QtWidgets>
#ifndef QT_NO_PRINTDIALOG
//#include <QtPrintSupport>
#endif

#include <QDebug>
#include <QMessageBox>
#include <QMenu>
#include <QtGlobal>

class QAction;
class QVectorWidget;
class QMenu;
class QTextEdit;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //explicit MainWindow(QWidget *parent = 0);
    MainWindow();
    ~MainWindow();

private slots:
    void about();
    void chooseActorColors();
    void importActorColors();
    void exportActorColors();
    void resetActorColors();
    void updateColors(QVector<QColor> updatedColors);
    void dockWindowChanged();
    //CSV
    void setCSVItemModel(QStandardItemModel * model, QStringList scenarioName);
    void csvGetFilePAth(bool bl, QString filepath=0);
    //Database
    void setDBItemModel(QStandardItemModel *model);
    void setDBItemModelEdit();
    void displayMessage(QString cls, QString message);
    void dbGetFilePAth(bool bl, QString dbPath =0,bool run = false );
    void dbEditGetFilePAth(bool bl);
    void updateStateCountSliderRange(int states);
    void updateScenarioListComboBox(QStringList * scenarios, QStringList *scenarioIds, QStringList *scenarioDesc, int indx);
    void updateDimensionCount(int dim, QStringList *dimList);
    void updateActorCount(int actNum);

    //Central-  Controls Frame
    void sliderStateValueToQryDB(int value);
    void scenarioComboBoxValue(int scenarioBox);
    void createNewSMPData(bool bl);
    void cellSelected(QStandardItem *in);
    void insertNewRowCSV();
    void deleteLastRow();
    void insertNewColumnCSV();
    void donePushButtonClicked(bool bl);
    void saveTableWidgetToCSV(bool bl);

    //    bool eventFilter(QObject*, QEvent*);
    void displayMenuTableWidget(QPoint pos);
    void displayMenuTableView(QPoint pos);
    //    void displayAffinityMenuTableWidget(QPoint pos);
    //    void displayCsvAffinityMenuTableView(QPoint pos);

    //DB to CSV
    void actorsNameDesc(QVector<QString> actorName, QVector<QString> actorDescription);
    void actorsInfluence(QVector<QString> ActorInfluence);
    void actorsPosition(QVector<QString> actorPosition, int dim);
    void actorsSalience(QVector<QString> actorSalience, int dim);
    void actAffinity(QVector<QString> actorAff, QVector<int> actorI, QVector<int> actorJ);
    void scenarioModelParameters(QVector<int> modParaDB, QString seedDB);

signals:
    //CSV
    void csvFilePath(QString path);

    //Database
    void dbFilePath(QString path,QString dbType, QString connection ,bool runScenario = false);
    void getScenarioRunValues(int state, QString scenarioBox,int dim);
    void getScenarioRunValuesEdit(QString scenarioBox);
    void getStateCountfromDB();
    void getDimensionCountfromDB();
    void getDimforBar();

    void releaseDatabase();
    void getPostgresDBList(QString,bool);

    //save DB to csv
    void getActorsDesc();
    void getInfluence(int turn);
    void getPosition(int dim,int turn);
    void getSalience(int dim,int turn);

    //colors
    void exportColors(QString path, QVector<int> actorIds, QVector<QString> colorCode);
    void importColors(QString path, int actorCount);

private:
    //MainWindow
    void createActions();
    void createStatusBar();
    void createLinePlotsDockWindows();
    void createBarPlotsDockWindows();
    void createQuadMapDockWindows();
    void createModelParametersDockWindow();
    void saveTableViewToCSV();
    void saveAsDialog();
    int validateControlButtons(QString viewName);
    QString getImageFileName(QString imgType, QString imgFileName, QString imgExt);

    void updateDBViewColumns();
    QString checkForHeaderString(QString header);
    void changesActorsStyleSheet();

    // Central Main Frame
    QFrame *central;
    QFrame *tableControlsFrame;
    QGridLayout *gLayout;
    QStackedWidget * stackWidget;

    QComboBox * scenarioComboBox;
    QPushButton * addActorsPushButton;
    QPushButton * deleteActorsPushButton;
    QLineEdit * actorsLineEdit;
    QPushButton * dimensionsPushButton;
    QLineEdit * dimensionsLineEdit;
    QPushButton * donePushButton;
    QLineEdit * scenarioDescriptionLineEdit;
    QSlider * turnSlider;
    QLineEdit * scenarioNameLineEdit;

    //Model parameters
    QFrame * frames;
    QPushButton * runButton;

    QComboBox * victProbModelComboBox;
    QComboBox * pCEModelComboBox;
    QComboBox * stateTransitionsComboBox;
    QComboBox * votingRuleComboBox;
    QComboBox * bigRAdjustComboBox;
    QComboBox * bigRRangeComboBox;
    QComboBox * thirdPartyCommitComboBox;
    QComboBox * interVecBrgnComboBox;
    QComboBox * bargnModelComboBox;

    int victProbModel;
    int pCEModel;
    int stateTransitions;
    int votingRule;
    int bigRAdjust;
    int bigRRange;
    int thirdPartyCommit;
    int interVecBrgn;
    int bargnModel;

    std::vector<int> parameters;

    //csv window
    QTabWidget * csvTableViewTabWidget;
    QTableView * csvTableView;
    QTableView * csvTableAffinityView;
    QStandardItemModel * csvAffinityModel;
    CSV *csvObj;

    QTableWidget * csvTableWidget;
    QTabWidget * smpDataTab;
    QTableWidget * affinityMatrix;

    QMenu *viewMenu;

    QDockWidget * modelParametersDock;
    QDockWidget * lineGraphDock;
    QDockWidget * barGraphDock;
    QDockWidget * quadMapDock;

    QGridLayout * VLayout;
    QFrame * modelParametersFrame;

    //Database Obj
    Database * dbObj ;
    QSqlDatabase db;
    int dimensions;
    QString dbPath;

    //Graph 1 widget
    QFrame *lineGraphMainFrame;
    QGridLayout * lineGraphGridLayout;
    QCustomPlot * lineCustomGraph;

    void initializeLineGraphPlot();
    void plotGraph();
    void initializeCentralViewFrame();

    QString scenarioBox;
    QStringList mScenarioIds;
    QStringList mScenarioDesc;
    QStringList mScenarioName;
    //graph - customplot

    int numAct;
    int prevTurn;
    bool firstVal;
    //    to  edit headers

    QLineEdit* headerEditor;
    int editorIndex;

    QString  tableType; // CSV, Database, NewSMPData
    QStandardItemModel *modeltoCSV;
    QStandardItemModel *modeltoDB;

    QString inputCSV;
    void createSeperateColumn(QTableWidgetItem *hdr);

    //DB to CSV
    int dimension;
    QVector <QString> actorsName;
    QVector <QString> actorsDescription;
    QVector <QString> actorsInfl;
    QVector <QString> actorsPos[50];
    QVector <QString> actorsSal[50];
    QVector <QString> actorAffinity;
    QVector <int>     actI;
    QVector <int>     actJ;

    QStringList dimensionList;

private slots:
    void titleDoubleClick(QMouseEvent *event, QCPPlotTitle *barGraphTitle);
    //    void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    //    void legendDoubleClick(QCPLegend* legend, QCPAbstractLegendItem* item);
    //    void selectionChanged();
    void mousePress();
    void mouseWheel();
    void addGraphOnLinePlot(const QVector<double> &x, const QVector<double> &y, QString Actor, int turn);
    //    void removeSelectedGraph();
    void removeAllGraphs();
    void moveLegend();
    void graphClicked(QCPAbstractPlottable *plottable, QMouseEvent* e);
    void graphDoubleClicked(QCPAbstractPlottable *plottable, QMouseEvent* e);
    void updateBarDimension(QStringList* dims);
    void linePlotContextMenuRequest(QPoint pos);
    void saveLinePlotAsBMP();
    void saveLinePlotAsPDF();

    //Bar Charts
private :
    void initializeBarGraphDock();
    void initializeBarGraphPlot();
    void populateBarGraphActorsList();
    void populateBarGraphDimensions(int dim);
    void populateBarGraphStateRange(int states);
    void generateColors();
    void getActorsInRange(int dim);
    void setStackedBars();
    void deleteBars();

    QCPBars *createBar(int actorId);

    QFrame * barGraphControlsFrame;
    QFrame * barGraphMainFrame;
    QGridLayout * barGraphGridLayout;
    QGridLayout * barGridLayout;
    QScrollArea * barGraphActorsScrollArea;
    QCustomPlot * barCustomGraph;

    QCheckBox * barGraphSelectAllCheckBox;
    QRadioButton * barGraphRadioButton;
    QComboBox * barGraphDimensionComboBox;
    QLineEdit * barGraphGroupRangeLineEdit;

    QSlider * barGraphTurnSlider;
    QVector <QCheckBox *> barGraphActorsCheckBoxList;
    QVector <bool> barGraphCheckedActorsIdList;
    QPushButton * barGraphBinWidthButton;


    QVector<QCPBars *> bars[100];
    QCPBars * prevBar;
    QCPBars * prevBarU;
    QVector<QColor> colorsList;

    QVector <QCheckBox * > barActorCBList;
    QVector <int> actorsIdsClr;

    double yAxisLen;

    int in;
    int barsCount;

    double currentStackHeight[100];
    double binWidth;

    QCPPlotTitle * barGraphTitle;

signals :
    void getActorIdsInRange(double lowerRange, double upperRange, int dimension, int turn);

private slots:
    void barGraphSelectAllActorsCheckBoxClicked(bool Click);
    void barGraphActorsCheckboxClicked(bool click);
    void barGraphDimensionChanged(int value);
    void barGraphTurnSliderChanged(int value);
    void barGraphBinWidthButtonClicked(bool bl);
    void barGraphActorsSalienceCapability(QVector<int> aId, QVector<double> sal, QVector<double>cap, double r1, double r2);
    void xAxisRangeChanged( const QCPRange &newRange, const QCPRange &oldRange );
    void yAxisRangeChanged( const QCPRange &newRange, const QCPRange &oldRange );
    void barPlotContextMenuRequest(QPoint pos);
    void saveBarPlotAsBMP();
    void saveBarPlotAsPDF();

    //line Graph
private :
    void initializeLineGraphDock();
    void populateLineGraphActorsList();
    void populateLineGraphDimensions(int dim);
    void populateLineGraphStateRange(int states);
    void createSpline(const QVector<double> &x, const QVector<double> &y, QString Actor, int turn);

    void clearAllLabels();
    void clearAllGraphs();
    void reconnectPlotWidgetSignals();

    QFrame * lineGraphControlsFrame;
    QScrollArea *lineGraphActorsScrollArea;
    QCheckBox *lineGraphSelectAllCheckBox;
    QRadioButton *lineGraphRadioButton ;
    QRadioButton *sankeyGraphRadioButton;
    QComboBox *lineGraphDimensionComboBox;
    QStackedWidget *graphTypeStackedWidget;
    QSlider *lineGraphTurnSlider;

    QVector <QCheckBox *> lineGraphActorsCheckBoxList;
    QVector <bool> lineGraphCheckedActorsIdList;

    QVector <QCheckBox * > lineActorCBList;

    QCPPlotTitle * lineGraphTitle;

    int numStates;

    QVector <bool> lineLabelToggleList;
    QVector <QCPItemText * > lineLabelList;
    QCPItemText *textLabel ;
    int tnty;

private slots :
    void lineGraphSelectAllActorsCheckBoxClicked(bool click);
    void lineGraphDimensionChanged(int value);
    void lineGraphTurnSliderChanged(int);
    void lineGraphActorsCheckboxClicked(bool click);
    void updateLineDimension(QStringList *dims);
    void toggleLabels();
    void splineValues(const QVector<double> &x, const QVector<double> &y);

    //run smp
private :
    QString csvPath;
    QCheckBox * logMinimum;
    QLineEdit * seedRand;

    void initializeModelParametersDock();
    void getParametersValues();
    //    QStandardItemModel *initializeComboBox(int rows, QStringList items);

private slots :
    void runPushButtonClicked(bool bl);
    void runModel(QString conStr,QString fileName =0);
    void smpDBPath(QString smpdbPath);
    void disableRunButton(QTableWidgetItem * itm);

    //Log
private :
    QString logFileName;
    QString defaultLogFile;

    QActionGroup * logActions;

    QAction *logDefaultAct;
    QAction *logNewAct;
    QAction *logNoneAct;

    std::streambuf *coutbuf;
    FILE *stream ;
    FILE fp_old;

    void logSMPDataOptionsAnalysis();

    //Quadmap
private :
    QFrame * quadMapMainFrame;
    QGridLayout * quadMapGridLayout;
    QCustomPlot * quadMapCustomGraph;
    QCPPlotTitle * quadMapTitle;
    QScrollArea * quadMapInitiatorsScrollArea;
    QScrollArea * quadMapReceiversScrollArea;
    QFrame * quadMapPerspectiveFrame;

    QVector <QRadioButton *> quadMapInitiatorsRadioButtonList;
    QVector <QCheckBox *> quadMapReceiversCheckBoxList;
    QVector <bool> quadMapReceiversCBCheckedList;

    QComboBox * vComboBox;
    QComboBox * hComboBox;

    QCheckBox * selectAllReceiversCB;

    QComboBox * perspectiveComboBox;
    QSlider * quadMapTurnSlider;

    QVector <double> deltaUtilV;
    QVector <double> deltaUtilH;
    int actorIdIndexV;
    QVector <int> actorIdIndexH;

    int actorsQueriedCount;

    QCPItemRect *   xRectItemPP;
    QCPItemRect *   xRectItemMP;
    QCPItemRect *   xRectItemMM;
    QCPItemRect *   xRectItemPM;

    QCheckBox * autoScale;
    QString prevScenario;

    QVector <int> VHAxisValues;

    QPushButton * plotQuadMap;
    int initiatorTip;

    bool useHistory;
    bool sankeyOutputHistory;
    QString currentScenarioId;

    void initializeQuadMapDock();
    void initializeQuadMapPlot();

    void populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
    void populatePerspectiveComboBox();
    void populateQuadMapStateRange(int states);
    void getUtilChlgHorizontalVerticalAxisData(int turn);
    void getUtilChlgHorizontalAxisData(int turn);
    void plotScatterPointsOnGraph(QVector<double> x, QVector<double> y, int actIndex);
    void plotDeltaValues();
    void removeAllScatterPoints();

private slots :
    void populateVHComboBoxPerspective(int index);
    void populateHcomboBox(QString vComboBoxText);

    void initiatorsChanged(bool bl);
    void receiversChanged(bool bl);

    void selectAllReceiversClicked(bool bl);
    void quadMapTurnSliderChanged(int turn);

    void quadMapUtilChlgandSQValues(int turn, double hor, double ver,
                                    int actorID);

    void xAxisRangeChangedQuad(QCPRange newRange, QCPRange oldRange);
    void yAxisRangeChangedQuad(QCPRange newRange, QCPRange oldRange);

    void quadMapAutoScale(bool status);
    void quadMapPlotPoints(bool status);
    void dbImported(bool bl);
    void quadPlotContextMenuRequest(QPoint pos);
    void saveQuadPlotAsBMP();
    void saveQuadPlotAsPDF();

signals :
    void getUtilChlgAndUtilSQfromDB(QVector <int > VHAxisList);


    //Xmlparser
private :
    QString xmlPath;
    Xmlparser *xmlparser;
    QTabWidget *xmlTabWidget;
    QTableView *xmlImportDataTableView;
    QTableView *xmlAffinityMatrixTableView;

    QStandardItemModel * xmlSmpDataModel;
    QStandardItemModel * xmlAffinityMatrixModel;

    QStringList dimensionsXml;
    QVector<int> defParameters;

    QAbstractButton * CSVButton;
    QAbstractButton * XMLButton;
    QDialogButtonBox * saveAsDial;

    bool savedAsXml;

    void createXmlTable();
    void populateXmlTable(QStandardItemModel *actorsVal);
    void populateAffinityMatrix(QVector<QStringList> idealAdj, QVector<QString> actors);
    void updateControlsBar(QStringList modelDesc);
    void updateModelParameters(QStringList modPara);
    void initializeAffinityMatrixRowCol(int count, QString table);
    void saveTableViewToXML();
    void setDefaultParameters();
    void saveTurnHistoryToCSV();

private slots:
    void displayMenuXmlTableView(QPoint pos);
    //    void displayMenuXmlAffTableView(QPoint pos);
    void importXmlGetFilePath(bool bl, QString filepath=0);
    void openStatusXml(bool status);
    void xmlDataParsedFromFile(QStringList modelDesc, QStringList modpara, QStringList dims,
                               QStandardItemModel *actModel, QVector <QStringList> idealAdj);
    void savedXmlName(QString fileName);
    void saveTableWidgetToXML(bool bl);
    void xmlCellSelected(QStandardItem *in);
signals:
    bool openXMLFile(QString file);
    void readXMLFile();
    void saveXMLDataToFile(QStringList parameters, QStandardItemModel * smpData,
                           QStandardItemModel * affModel, QString path);
    void saveNewSMPDataToXMLFile(QStringList parameters, QTableWidget
                                 * smpDataWidget, QTableWidget * affModelWidget);
    void homeDirChanged(QString dir);

    //actormoved
private:
    QStandardItemModel * actorMovedDataModel;
    QVector <int> actorIDList;
    QVector <int> actorTurnList;
    QVector <int> actorDimensionList;
signals:
    void getActorMovedData(QString scenario);

private slots :
    void actorMovedInfoModel(QStandardItemModel * actorMovedData);

private :
    PopupWidget * popup;
    QMenu *fileMenu;
    QString homeDirectory;
    QString defaultDirectory;
    QSettings recentFileSettings;

    //recentfile history
    QAction *separatorAct;

    enum { maxRecentFilesCount = 5 };
    QAction *recentFileActs[maxRecentFilesCount];

    void setCurrentFile(const QString &fileName);
    void updateRecentFileActions();
    QString strippedName(const QString &fullFileName);
    void loadRecentFile(const QString &fileName);
    void intializeHomeDirectory();
    void checkCSVtype(QString fileName);

private slots:
    void openRecentFile();
    void clearRecentFile(bool bl);
    void changeHomeDirectory(bool bl);

    //Databse dialog
private :
    QString connectionString;
    DatabaseDialog * dbDialog;
    QLineEdit * dbDialogName;
    QString generateTimeStamp();
    bool menuconfig;
    QString recentFileAccess;
    bool importedDBFile;

    void postgresRecentAccess();
    void removeFromRecentFileHistory(QString fileName);

private slots:
    void connectionStrPath(QString str);
    void configureDB(bool bl);
    void postgresDBList(QStringList* dbList, bool imp);
    void dbListClicked(QModelIndex indx);
    void dbDonePushButtonClicked(bool bl);
    void getTimeStamp(bool bl);
    void configUsingMenu(bool bl);

};



#endif // MAINWINDOW_H


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
