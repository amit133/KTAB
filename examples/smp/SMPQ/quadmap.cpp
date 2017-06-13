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
#include "smp.h"

using SMPLib::SMPModel;
using SMPLib::SMPActor;
using SMPLib::SMPState;
void MainWindow::initializeQuadMapDock()
{
    quadMapCustomGraph= new QCustomPlot;
    quadMapGridLayout->addWidget(quadMapCustomGraph,0,0,1,1);
    quadMapGridLayout->setRowStretch(0,2);
    initializeQuadMapPlot();

    QHBoxLayout * hlay = new QHBoxLayout;
    plotQuadMap = new QPushButton(" Plot ");
    hlay->addWidget(plotQuadMap);
    connect(plotQuadMap,SIGNAL(clicked(bool)),this,SLOT(quadMapPlotPoints(bool)));

    autoScale = new QCheckBox("Auto Scale");
    autoScale->setToolTip("The axes limits on the quad map default to [-1,1]; "
                          " \ncheck this to zoom on the range of the plotted data ");
    hlay->addWidget(autoScale);

    connect(autoScale,SIGNAL(clicked(bool)),this,SLOT(quadMapAutoScale(bool)));

    quadMapGridLayout->addLayout(hlay,2,0,Qt::AlignLeft);

    QFrame * quadMapControlsFrame = new QFrame;
    quadMapControlsFrame->setFrameShape(QFrame::StyledPanel);
    quadMapControlsFrame->setMaximumHeight(175);
    quadMapGridLayout->addWidget(quadMapControlsFrame,3,0,Qt::AlignBottom);

    QGridLayout *quadMapControlsLayout = new QGridLayout(quadMapControlsFrame);

    QFont  labelFont;
    labelFont.setBold(true);

    QLabel * initiatorsLabel = new QLabel("Initiator");
    initiatorsLabel->setAlignment(Qt::AlignHCenter);
    initiatorsLabel->setFont(labelFont);
    initiatorsLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
    quadMapControlsLayout->addWidget(initiatorsLabel,0,0,Qt::AlignBottom);

    quadMapInitiatorsScrollArea = new QScrollArea(quadMapControlsFrame);
    quadMapControlsLayout->addWidget(quadMapInitiatorsScrollArea,1,0,2,1);

    QLabel * receiversLabel = new QLabel("Receiver(s)");
    receiversLabel->setAlignment(Qt::AlignHCenter);
    receiversLabel->setFont(labelFont);
    receiversLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
    quadMapControlsLayout->addWidget(receiversLabel,0,1,Qt::AlignBottom);

    quadMapReceiversScrollArea = new QScrollArea(quadMapControlsFrame);
    quadMapControlsLayout->addWidget(quadMapReceiversScrollArea,1,1,Qt::AlignBottom);
    selectAllReceiversCB = new QCheckBox("Select All");
    selectAllReceiversCB->setChecked(true);
    quadMapControlsLayout->addWidget(selectAllReceiversCB,2,1);
    connect(selectAllReceiversCB,SIGNAL(clicked(bool)),this,SLOT(selectAllReceiversClicked(bool)));

    QLabel * perspectiveLabel = new QLabel("Perspective");
    perspectiveLabel->setAlignment(Qt::AlignHCenter);
    perspectiveLabel->setFont(labelFont);
    perspectiveLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
    quadMapControlsLayout->addWidget(perspectiveLabel,0,2);

    quadMapPerspectiveFrame = new QFrame(quadMapControlsFrame);
    quadMapPerspectiveFrame->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
    quadMapControlsLayout->addWidget(quadMapPerspectiveFrame,1,2,2,1);

    quadMapTurnSlider  = new QSlider(Qt::Horizontal);
    quadMapTurnSlider->setTickInterval(1);
    quadMapTurnSlider->setTickPosition(QSlider::TicksBothSides);
    quadMapTurnSlider->setPageStep(1);
    quadMapTurnSlider->setSingleStep(1);
    quadMapTurnSlider->setRange(0,1);
    quadMapTurnSlider->setVisible(false);
    connect(quadMapTurnSlider,SIGNAL(valueChanged(int)),this,SLOT(quadMapTurnSliderChanged(int)));

    populatePerspectiveComboBox();
}

void MainWindow::initializeQuadMapPlot()
{
    QFont font("Helvetica[Adobe]",10);
    quadMapTitle = new QCPPlotTitle(quadMapCustomGraph,"Quad Map");
    quadMapTitle->setFont(font);
    quadMapTitle->setTextColor(QColor(0,128,0));

    quadMapCustomGraph->plotLayout()->insertRow(0);
    quadMapCustomGraph->plotLayout()->addElement(0, 0, quadMapTitle);

    quadMapCustomGraph->xAxis->setAutoTicks(true);
    quadMapCustomGraph->xAxis->setAutoTickLabels(true);
    quadMapCustomGraph->xAxis->setRange(-1,1);
    quadMapCustomGraph->xAxis->setSubTickCount(0);
    quadMapCustomGraph->xAxis->setTickLength(0,0.1);
    quadMapCustomGraph->xAxis->grid()->setVisible(true);
    quadMapCustomGraph->xAxis->setLabel("E[ΔU] to Receiver");

    quadMapCustomGraph->yAxis->setAutoTicks(true);
    quadMapCustomGraph->yAxis->setAutoTickLabels(true);

    quadMapCustomGraph->yAxis->setRange(-1, 1);
    quadMapCustomGraph->yAxis->setPadding(0); // a bit more space to the left border
    quadMapCustomGraph->yAxis->setLabel("E[ΔU] to Initiator");
    quadMapCustomGraph->yAxis->grid()->setSubGridVisible(false);

    connect(quadMapCustomGraph->xAxis, SIGNAL(rangeChanged(QCPRange,QCPRange)), this, SLOT(xAxisRangeChangedQuad(QCPRange,QCPRange)));
    connect(quadMapCustomGraph->yAxis, SIGNAL(rangeChanged(QCPRange,QCPRange)), this, SLOT(yAxisRangeChangedQuad(QCPRange,QCPRange)));

    // setup legend:
    quadMapCustomGraph->legend->setVisible(false);
    quadMapCustomGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    // create a rectItem andchange the background color of plot
    xRectItemPP = new QCPItemRect( quadMapCustomGraph);
    xRectItemPP->setVisible(true);
    xRectItemPP->setPen(QPen(Qt::transparent));
    xRectItemPP->setBrush(QBrush(QColor(0,255,0,75)));
    xRectItemPP->topLeft->setType(QCPItemPosition::ptPlotCoords);
    xRectItemPP->topLeft->setCoords(0,500); // +y
    xRectItemPP->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    xRectItemPP->bottomRight->setCoords(500, 0); // +x
    xRectItemPP->setClipToAxisRect(true);

    xRectItemMP = new QCPItemRect(quadMapCustomGraph);
    xRectItemMP->setVisible(true);
    xRectItemMP->setPen(QPen(Qt::transparent));
    xRectItemMP->setBrush(QBrush(QColor(255,0,0,75)));
    xRectItemMP->topLeft->setType(QCPItemPosition::ptPlotCoords);
    xRectItemMP->topLeft->setCoords(0,500);// y
    xRectItemMP->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    xRectItemMP->bottomRight->setCoords(-500, 0);// -x
    xRectItemMP->setClipToAxisRect(true);

    xRectItemMM = new QCPItemRect(quadMapCustomGraph);
    xRectItemMM->setVisible(true);
    xRectItemMM->setPen(QPen(Qt::transparent));
    xRectItemMM->setBrush(QBrush(QColor(211,211,211,75)));
    xRectItemMM->topLeft->setType(QCPItemPosition::ptPlotCoords);
    xRectItemMM->topLeft->setCoords(0,-500);// -y
    xRectItemMM->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    xRectItemMM->bottomRight->setCoords(-500, 0);// -x
    xRectItemMM->setClipToAxisRect(true);

    xRectItemPM = new QCPItemRect(quadMapCustomGraph);
    xRectItemPM->setVisible(true);
    xRectItemPM->setPen(QPen(Qt::transparent));
    xRectItemPM->setBrush(QBrush(QColor(255,255,0,75)));
    xRectItemPM->topLeft->setType(QCPItemPosition::ptPlotCoords);
    xRectItemPM->topLeft->setCoords(0,-500);// -y
    xRectItemPM->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    xRectItemPM->bottomRight->setCoords(500, 0);// +x
    xRectItemPM->setClipToAxisRect(true);

    // setup policy and connect slot for context menu popup:
    quadMapCustomGraph->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(quadMapCustomGraph, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(quadPlotContextMenuRequest(QPoint)));
}

void MainWindow::populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes()
{
    QRadioButton *initiatorRB;
    QCheckBox *receiversCB;
    quadMapInitiatorsRadioButtonList.clear();
    quadMapReceiversCheckBoxList.clear();
    quadMapReceiversCBCheckedList.clear();

    QWidget* widgetRB = new QWidget;
    QVBoxLayout *layoutRB = new QVBoxLayout(widgetRB);

    QWidget* widgetCB = new QWidget;
    QVBoxLayout *layoutCB = new QVBoxLayout(widgetCB);

    for(int actorsCount = 0; actorsCount < actorsName.count(); ++actorsCount)
    {
        initiatorRB = new QRadioButton(actorsName.at(actorsCount));
        receiversCB = new QCheckBox(actorsName.at(actorsCount));

        if(0==actorsCount)
        {
            initiatorRB->setChecked(true);
            receiversCB->setVisible(false);
        }
        receiversCB->setChecked(true);

        QColor mycolor = colorsList.at(actorsCount);

        QString style = "background: rgb(%1, %2, %3);";
        style = style.arg(mycolor.red()).arg(mycolor.green()).arg(mycolor.blue());
        style += "color:white; font-size:15px;";
        style += "font-weight:bold;";

        initiatorRB->setStyleSheet(style);
        receiversCB->setStyleSheet(style);

        initiatorRB->setObjectName(QString::number(actorsCount));
        receiversCB->setObjectName(QString::number(actorsCount));

        layoutRB->addWidget(initiatorRB);
        layoutCB->addWidget(receiversCB);
        layoutRB->stretch(0);
        layoutCB->stretch(0);

        connect(initiatorRB,SIGNAL(clicked(bool)),this,SLOT(initiatorsChanged(bool)));
        connect(receiversCB,SIGNAL(clicked(bool)),this,SLOT(receiversChanged(bool)));

        quadMapInitiatorsRadioButtonList.append(initiatorRB);
        quadMapReceiversCheckBoxList.append(receiversCB);

        //setting all checkboxes as checked as initial condition
        quadMapReceiversCBCheckedList.append(true);

    }
    quadMapInitiatorsScrollArea->setWidget(widgetRB);
    quadMapReceiversScrollArea->setWidget(widgetCB);

    perspectiveComboBox->currentIndexChanged(perspectiveComboBox->currentIndex());

    quadMapInitiatorsScrollArea->setToolTip("Plot expected utility changes for possible bargains initiated by this actor");
    quadMapReceiversScrollArea->setToolTip("Plot expected utility changes for possible bargains received by these actors");

    widgetRB->adjustSize();
    widgetCB->adjustSize();
}

void MainWindow::populatePerspectiveComboBox()
{
    perspectiveComboBox = new QComboBox;
    perspectiveComboBox->setToolTip("Actors from whose perspectives the expected utility "
                                    "\nchanges are computed and plotted in each axis");
    QWidget* widget = new QWidget;
    QGridLayout *layout = new QGridLayout(widget);

    QStringList items;
    items << "Initiator" <<"Receiver(s)" <<"Objective" <<"Other";
    perspectiveComboBox->addItems(items);
    perspectiveComboBox->setItemData(0,"Compute all utility changes from the perspective of the initiating actor"
                                     ,Qt::ToolTipRole);
    perspectiveComboBox->setItemData(1,"Compute all utility changes from the perspective of the receiving actor(s)"
                                     ,Qt::ToolTipRole);
    perspectiveComboBox->setItemData(2,"The vertical axis shows the utility changes from the perspective of the "
                                       "\ninitiating actor, while the horizontal axis shows utility changes from "
                                       "\nthe perspective of the receiving actor(s)"
                                     ,Qt::ToolTipRole);
    perspectiveComboBox->setItemData(3,"Allows the selection of any actor from whose perspective the utility changes"
                                       "\n are computed and shown on both axes"
                                     ,Qt::ToolTipRole);

    layout->addWidget(perspectiveComboBox,0,0,0,-1,Qt::AlignTop);

    QFont  labelFont;
    labelFont.setBold(true);

    QVBoxLayout * vLay = new QVBoxLayout;
    QVBoxLayout * hLay = new QVBoxLayout;

    QLabel *vLabel = new QLabel("V");
    vLabel->setAlignment(Qt::AlignHCenter);
    vLabel->setFont(labelFont);
    vLabel->setToolTip("Actor from whose perspective the utility changes \non the vertical axis are computed");
    vLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);

    QLabel *hLabel = new QLabel("H");
    hLabel->setAlignment(Qt::AlignHCenter);
    hLabel->setFont(labelFont);
    hLabel->setToolTip("Actor from whose perspective the utility changes \non the horizontal axis are computed");
    hLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);

    vComboBox = new QComboBox;
    hComboBox = new QComboBox;

    vLay->addWidget(vLabel);
    vLay->addWidget(vComboBox);

    hLay->addWidget(hLabel);
    hLay->addWidget(hComboBox);

    layout->addLayout(vLay,1,0);
    layout->addLayout(hLay,1,1);

    layout->setSizeConstraint(QLayout::SetMinimumSize);
    quadMapPerspectiveFrame->setLayout(layout);

    connect(perspectiveComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(populateVHComboBoxPerspective(int)));
    perspectiveComboBox->setCurrentIndex(2);

    connect(vComboBox,SIGNAL(currentIndexChanged(QString)),this, SLOT(populateHcomboBox(QString)));

    perspectiveComboBox->setMinimumWidth(perspectiveComboBox->minimumSizeHint().width()-20);
    vComboBox->setMinimumWidth(vComboBox->minimumSizeHint().width()-20);
    vComboBox->setToolTip("Actor from whose perspective the utility changes\n on the vertical axis are computed");
    hComboBox->setMinimumWidth(hComboBox->minimumSizeHint().width()-20);
    hComboBox->setToolTip("Actor from whose perspective the utility changes\n on the horizontal axis are computed");

    widget->adjustSize();
}

void MainWindow::populateQuadMapStateRange(int states)
{
    quadMapTurnSlider->setRange(0,states);
    connect(turnSlider,SIGNAL(valueChanged(int)),quadMapTurnSlider,SLOT(setValue(int)));
}

void MainWindow::getUtilChlgHorizontalVerticalAxisData(int turn)
{
    deltaUtilV.clear();
    deltaUtilH.clear();

    int affK=0;
    int estH=0;
    int initI=0;

    for(int initiatorIndex=0; initiatorIndex < actorsName.length(); ++ initiatorIndex)
    {
        if(true==quadMapInitiatorsRadioButtonList.at(initiatorIndex)->isChecked())
            initI = initiatorIndex;

        initiatorTip=initI;
    }

    for(int recdJ =0; recdJ < actorsName.length(); ++recdJ)
    {
        if(true==quadMapReceiversCheckBoxList.at(recdJ)->isChecked()
                && true == quadMapReceiversCheckBoxList.at(recdJ)->isVisible())
        {
            if(0==perspectiveComboBox->currentIndex()) // initiators
            {
                VHAxisValues.clear();
                estH=affK=initI;
                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);

                estH=initI;
                affK=recdJ;
                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);
            }
            else if(1==perspectiveComboBox->currentIndex()) // receivers
            {
                VHAxisValues.clear();
                affK=initI;
                estH=recdJ;
                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);

                estH=affK=recdJ;
                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);
            }
            else if(2==perspectiveComboBox->currentIndex()) //objective
            {
                VHAxisValues.clear();
                estH=affK=initI;
                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);

                estH=affK=recdJ;
                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);
            }
            else //others
            {
                VHAxisValues.clear();

                affK=initI;
                estH=vComboBox->currentIndex()-1; // -1, actors index starts from 1 not zero, only here.

                if(estH<0)
                    return;

                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);

                VHAxisValues.append(turn);
                VHAxisValues.append(estH);
                VHAxisValues.append(affK);
                VHAxisValues.append(initI);
                VHAxisValues.append(recdJ);
            }
            //            emit getUtilChlgAndUtilSQfromDB(VHAxisValues);
            double x,y;
            if(useHistory)
            {
                y =SMPLib::SMPModel::getQuadMapPoint(VHAxisValues.at(0),VHAxisValues.at(1),VHAxisValues.at(2),
                                                     VHAxisValues.at(3),VHAxisValues.at(4));
                x =SMPLib::SMPModel::getQuadMapPoint(VHAxisValues.at(5),VHAxisValues.at(6),VHAxisValues.at(7),
                                                     VHAxisValues.at(8),VHAxisValues.at(9));
            }
            else
            {
                y =SMPLib::SMPModel::getQuadMapPoint(dbPath.toStdString(),scenarioBox.toStdString(),VHAxisValues.at(0),
                                                     VHAxisValues.at(1),VHAxisValues.at(2),VHAxisValues.at(3),
                                                     VHAxisValues.at(4));
                x =SMPLib::SMPModel::getQuadMapPoint(dbPath.toStdString(),scenarioBox.toStdString(),VHAxisValues.at(5),
                                                     VHAxisValues.at(6),VHAxisValues.at(7),VHAxisValues.at(8),
                                                     VHAxisValues.at(9));
                //                qDebug()<<VHAxisValues << scenarioBox;
            }

            quadMapUtilChlgandSQValues(VHAxisValues.at(0),x,y,VHAxisValues.at(4));
        }
    }
}

void MainWindow::plotScatterPointsOnGraph(QVector <double> x,QVector <double> y, int actIndex)
{

    quadMapCustomGraph->addGraph();
    quadMapCustomGraph->graph()->setData(x,y);
    quadMapCustomGraph->graph()->setLineStyle(QCPGraph::lsNone);
    quadMapCustomGraph->graph()->setScatterStyle( QCPScatterStyle::ssDisc);
    quadMapCustomGraph->graph()->setName(actorsName.at(actIndex));

    QString actorDetails;
    actorDetails.append("Name: <b>" + actorsName.at(actIndex) + "</b> <br>");
    actorDetails.append("Description: <b>" +actorsDescription.at(actIndex) + "</b> <br>");
    actorDetails.append("Perspective: <b>" +perspectiveComboBox->currentText() + "</b> <br>");
    actorDetails.append("Initiator: <b>" +actorsName.at(initiatorTip) + "</b> <br>");

    QString xcord;
    QString ycord;

    if(QString::number(x.at(0)).at(0).isNumber())
        xcord=QString::number(x.at(0)).left(5);
    else
        xcord=QString::number(x.at(0)).left(6);

    if(QString::number(y.at(0)).at(0).isNumber())
        ycord=QString::number(y.at(0)).left(5);
    else
        ycord=QString::number(y.at(0)).left(6);

    actorDetails.append("Coords: <b>" + xcord + ", " + ycord + "</b>");

    quadMapCustomGraph->graph()->setTooltip(actorDetails);

    QPen graphPen;
    graphPen.setColor(colorsList.at(actIndex));
    graphPen.setWidthF(2.0);

    quadMapCustomGraph->graph()->setPen(graphPen);

}

void MainWindow::plotDeltaValues()
{
    QVector <double> x;
    QVector <double> y;

    for(int i=0; i< actorsQueriedCount; ++i)
    {
        x.append(deltaUtilH.at(i));
        y.append(deltaUtilV.at(i));
        plotScatterPointsOnGraph(x,y,actorIdIndexH.at(i));
        x.clear();
        y.clear();
    }
}

void MainWindow::removeAllScatterPoints()
{
    quadMapCustomGraph->clearGraphs();
    quadMapCustomGraph->replot();
}

void MainWindow::populateVHComboBoxPerspective(int index)
{
    disconnect(vComboBox,SIGNAL(currentIndexChanged(QString)),this, SLOT(populateHcomboBox(QString)));
    if(0==index)
    {
        for(int i = 0; i <quadMapInitiatorsRadioButtonList.length();++i)
        {
            if(true==quadMapInitiatorsRadioButtonList.at(i)->isChecked())
            {
                vComboBox->clear();
                hComboBox->clear();
                vComboBox->addItem(quadMapInitiatorsRadioButtonList.at(i)->text());
                hComboBox->addItem(quadMapInitiatorsRadioButtonList.at(i)->text());
            }
        }
    }
    else if(1==index)
    {
        int count=0;
        for(int i = 0; i <quadMapReceiversCheckBoxList.length();++i)
        {
            if(true==quadMapReceiversCheckBoxList.at(i)->isChecked())
            {
                count++;
                if(count==1)
                {
                    vComboBox->clear();
                    hComboBox->clear();
                    vComboBox->addItem(quadMapReceiversCheckBoxList.at(i)->text());
                    hComboBox->addItem(quadMapReceiversCheckBoxList.at(i)->text());
                }
                else if(count>1)
                {
                    vComboBox->clear();
                    hComboBox->clear();
                    vComboBox->addItem("*");
                    hComboBox->addItem("*");
                }
                else
                {
                    vComboBox->clear();
                    hComboBox->clear();
                    vComboBox->addItem("-");
                    hComboBox->addItem("-");
                }
            }
            else if(count==0)
            {
                vComboBox->clear();
                hComboBox->clear();
                vComboBox->addItem("-");
                hComboBox->addItem("-");
            }
        }
    }
    else if(2==index)
    {
        int count=0;
        for(int i = 0; i <quadMapInitiatorsRadioButtonList.length();++i)
        {
            if(true==quadMapInitiatorsRadioButtonList.at(i)->isChecked())
            {
                vComboBox->clear();
                vComboBox->addItem(quadMapInitiatorsRadioButtonList.at(i)->text());
            }
        }
        for(int i = 0; i <quadMapReceiversCheckBoxList.length();++i)
        {
            if(true==quadMapReceiversCheckBoxList.at(i)->isChecked())
            {
                count++;
                if(count==1)
                {
                    hComboBox->clear();
                    hComboBox->addItem(quadMapReceiversCheckBoxList.at(i)->text());
                }
                else if(count>1)
                {
                    hComboBox->clear();
                    hComboBox->addItem("*");
                }
                else
                {
                    hComboBox->clear();
                    hComboBox->addItem("-");
                }
            }
            else if(count==0)
            {
                hComboBox->clear();
                hComboBox->addItem("-");
            }
        }
    }
    else
    {
        vComboBox->clear();
        hComboBox->clear();
        vComboBox->addItem(""
                           " ");
        for(int i = 0; i < actorsName.length(); ++i)
        {
            vComboBox->addItem(actorsName.at(i));
        }
    }

    // get the minimum width that fits the largest item.
    int width = vComboBox->minimumSizeHint().width();
    // set the ComboBox to that width.
    vComboBox->setMinimumWidth(width+20);
    hComboBox->setMinimumWidth(width+20);
    connect(vComboBox,SIGNAL(currentIndexChanged(QString)),this, SLOT(populateHcomboBox(QString)));

    perspectiveComboBox->setMinimumWidth(perspectiveComboBox->minimumSizeHint().width());
    vComboBox->setMinimumWidth(vComboBox->minimumSizeHint().width());
    hComboBox->setMinimumWidth(hComboBox->minimumSizeHint().width());

}

void MainWindow::populateHcomboBox(QString vComboBoxText)
{
    hComboBox->clear();
    hComboBox->addItem(vComboBoxText);

}

void MainWindow::initiatorsChanged(bool bl)
{
    Q_UNUSED(bl)
    actorsQueriedCount=0;
    perspectiveComboBox->currentIndexChanged(perspectiveComboBox->currentIndex());

    for(int actindex = 0; actindex<quadMapInitiatorsRadioButtonList.length();++actindex)
    {
        if(quadMapInitiatorsRadioButtonList.at(actindex)->isChecked())
            quadMapReceiversCheckBoxList.at(actindex)->setVisible(false);
        else
            quadMapReceiversCheckBoxList.at(actindex)->setVisible(true);

        if(quadMapReceiversCheckBoxList.at(actindex)->isVisible()
                && quadMapReceiversCheckBoxList.at(actindex)->isChecked())
            actorsQueriedCount++;
    }
}

void MainWindow::receiversChanged(bool bl)
{
    actorsQueriedCount=0;

    for(int actindex = 0; actindex<quadMapReceiversCheckBoxList.length();++actindex)
    {
        if(quadMapReceiversCheckBoxList.at(actindex)->isVisible()
                && quadMapReceiversCheckBoxList.at(actindex)->isChecked())
            actorsQueriedCount++;

        if(true==quadMapReceiversCheckBoxList.at(actindex)->isChecked())
            quadMapReceiversCBCheckedList[actindex]=true;
        else
            quadMapReceiversCBCheckedList[actindex]=false;
    }
    perspectiveComboBox->currentIndexChanged(perspectiveComboBox->currentIndex());
}

void MainWindow::selectAllReceiversClicked(bool bl)
{
    actorsQueriedCount=0;

    for(int actindex = 0; actindex<quadMapReceiversCheckBoxList.length();++actindex)
    {
        disconnect(quadMapReceiversCheckBoxList.at(actindex),SIGNAL(clicked(bool)),this,SLOT(receiversChanged(bool)));
        quadMapReceiversCBCheckedList[actindex]=bl;
        quadMapReceiversCheckBoxList.at(actindex)->setChecked(bl);
        connect(quadMapReceiversCheckBoxList.at(actindex),SIGNAL(clicked(bool)),this,SLOT(receiversChanged(bool)));

        if(quadMapReceiversCheckBoxList.at(actindex)->isVisible()
                && quadMapReceiversCheckBoxList.at(actindex)->isChecked())
            actorsQueriedCount++;
    }
    perspectiveComboBox->currentIndexChanged(perspectiveComboBox->currentIndex());
}

void MainWindow::quadMapTurnSliderChanged(int turn)
{
    actorsQueriedCount=0;
    for(int i=0; i < quadMapReceiversCheckBoxList.length() ; i++)
    {
        if(quadMapReceiversCheckBoxList.at(i)->isVisible() && quadMapReceiversCheckBoxList.at(i)->isChecked())
            actorsQueriedCount++;
    }
}

void MainWindow::quadMapUtilChlgandSQValues(int turn, double hor, double ver , int actorID)
{
    Q_UNUSED(turn)

    deltaUtilV.append(ver);
    deltaUtilH.append(hor);

    actorIdIndexH.append(actorID);

    if(actorsQueriedCount==deltaUtilV.length())
    {
        plotDeltaValues();
        actorIdIndexH.clear();
    }
}

void MainWindow::xAxisRangeChangedQuad(QCPRange newRange, QCPRange oldRange)
{
    if (newRange.upper > 100)
    {
        quadMapCustomGraph->xAxis->setRangeUpper(100);
        quadMapCustomGraph->xAxis->setRangeLower(-100);
    }
    if (newRange.upper < -100)
    {
        quadMapCustomGraph->xAxis->setRangeUpper(-100);
        quadMapCustomGraph->xAxis->setRangeLower(100);
    }
}

void MainWindow::yAxisRangeChangedQuad(QCPRange newRange, QCPRange oldRange)
{
    if (newRange.upper > 100)
    {
        quadMapCustomGraph->yAxis->setRangeUpper(100);
        quadMapCustomGraph->yAxis->setRangeLower(-100);
    }
    if (newRange.upper < -100)
    {
        quadMapCustomGraph->yAxis->setRangeUpper(-100);
        quadMapCustomGraph->yAxis->setRangeLower(100);
    }
}

void MainWindow::quadMapAutoScale(bool status)
{
    double vLower= *std::min_element(deltaUtilV.begin(), deltaUtilV.end());
    double vUpper= *std::max_element(deltaUtilV.begin(), deltaUtilV.end());

    double hLower = *std::min_element(deltaUtilH.begin(), deltaUtilH.end());
    double hUpper = *std::max_element(deltaUtilH.begin(), deltaUtilH.end());

    if(true==status)
    {
        quadMapCustomGraph->xAxis->setRange(hLower-0.02,hUpper+0.02);
        quadMapCustomGraph->yAxis->setRange(vLower-0.02,vUpper+0.02);
    }
    else
    {
        quadMapCustomGraph->xAxis->setRange(-1,1);
        quadMapCustomGraph->yAxis->setRange(-1,1);
    }
    quadMapCustomGraph->replot();
}

void MainWindow::quadMapPlotPoints(bool status)
{
    Q_UNUSED(status)
    if(true==quadMapDock->isVisible() && actorsName.length() >0)
    {
        QApplication::setOverrideCursor(QCursor(QPixmap("://images/hourglass.png"))) ;
        plotQuadMap->setEnabled(false);
        removeAllScatterPoints();
        getUtilChlgHorizontalVerticalAxisData(turnSlider->value());
        quadMapTitle->setText(QString(" E[ΔU] Quad Map for Actor %1, Turn "
                                      +QString::number(turnSlider->value())).arg(actorsName.at(initiatorTip)));
        quadMapCustomGraph->replot();
        if(true==autoScale->isChecked())
        {
            quadMapAutoScale(true);
        }
        else
        {
            quadMapAutoScale(false);
        }
        plotQuadMap->setEnabled(true);
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::dbImported(bool bl)
{
    useHistory=false;
    sankeyOutputHistory=true;
    importedDBFile=true;
    if(connectionString.contains("QPSQL"))
    {
        emit getPostgresDBList(connectionString,true); // true == imported, false == run
    }
}

void MainWindow::quadPlotContextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);

    menu->addAction("Save As BMP", this, SLOT(saveQuadPlotAsBMP()));
    menu->addAction("Save As PDF", this, SLOT(saveQuadPlotAsPDF()));

    menu->popup(quadMapCustomGraph->mapToGlobal(pos));
}

void MainWindow::saveQuadPlotAsBMP()
{
    QString fileName = getImageFileName("BMP File (*.bmp)","QuadMap",".bmp");
    if(!fileName.isEmpty())
    {
        quadMapCustomGraph->saveBmp(fileName);
        //        setCurrentFile(fileName);
    }
}


void MainWindow::saveQuadPlotAsPDF()
{
    QString fileName = getImageFileName("PDF File (*.pdf)","QuadMap",".pdf");
    if(!fileName.isEmpty())
    {
        quadMapCustomGraph->savePdf(fileName);
        //        setCurrentFile(fileName);
    }
}

