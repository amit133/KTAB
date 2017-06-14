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

//barchart - Graph 2 / Dock 2
void MainWindow::initializeBarGraphDock()
{
    barCustomGraph= new QCustomPlot;
    barGraphGridLayout->addWidget(barCustomGraph,1,0,1,1);
    barGraphGridLayout->setColumnStretch(0,2);
    initializeBarGraphPlot();

    barGraphControlsFrame = new QFrame;
    barGraphControlsFrame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout *barControlsVerticalLayout = new QVBoxLayout(barGraphControlsFrame);

    QFont  labelFont;
    labelFont.setBold(true);

    QLabel * actorsLabel = new QLabel("Actors");
    actorsLabel->setAlignment(Qt::AlignHCenter);
    actorsLabel->setFont(labelFont);
    actorsLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
    barControlsVerticalLayout->addWidget(actorsLabel);

    barGraphActorsScrollArea = new QScrollArea(barGraphControlsFrame);

    barControlsVerticalLayout->addWidget(barGraphActorsScrollArea);

    barGraphSelectAllCheckBox = new QCheckBox("Select All Actors");
    barGraphSelectAllCheckBox->setChecked(true);
    barControlsVerticalLayout->addWidget(barGraphSelectAllCheckBox);

    connect(barGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(barGraphSelectAllActorsCheckBoxClicked(bool)));

    QLabel * barGraphTypeLabel = new QLabel("Graph Type");
    barGraphTypeLabel->setAlignment(Qt::AlignHCenter);
    barGraphTypeLabel->setFont(labelFont);
    barGraphTypeLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);

    barControlsVerticalLayout->addWidget(barGraphTypeLabel);

    barGraphRadioButton = new QRadioButton("Stacked Bar Chart");
    barGraphRadioButton->setChecked(true);

    QHBoxLayout * layout = new QHBoxLayout;
    layout->addWidget(barGraphRadioButton);
    barControlsVerticalLayout->addLayout(layout);

    QLabel * barGraphDimensionsLabel = new QLabel("Dimensions");
    barGraphDimensionsLabel->setAlignment(Qt::AlignHCenter);
    barGraphDimensionsLabel->setFont(labelFont);
    barGraphDimensionsLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
    barControlsVerticalLayout->addWidget(barGraphDimensionsLabel);

    barGraphDimensionComboBox= new QComboBox;
    barControlsVerticalLayout->addWidget(barGraphDimensionComboBox);
    connect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));

    barGraphBinWidthButton = new QPushButton("No. of Bars");
    barGraphBinWidthButton->setFont(labelFont);
    barGraphBinWidthButton->setToolTip("Number of bars displayed in the bar plot");
    barControlsVerticalLayout->addWidget(barGraphBinWidthButton);
    connect(barGraphBinWidthButton,SIGNAL(clicked(bool)),this, SLOT(barGraphBinWidthButtonClicked(bool)));

    barGraphGroupRangeLineEdit = new  QLineEdit;
    barGraphGroupRangeLineEdit->setText(QString::number(10));
    barGraphGroupRangeLineEdit->setToolTip("Number of bars displayed in the bar plot");
    barControlsVerticalLayout->addWidget(barGraphGroupRangeLineEdit);

    barGraphTurnSlider = new QSlider(Qt::Horizontal);
    barGraphTurnSlider->setTickInterval(1);
    barGraphTurnSlider->setTickPosition(QSlider::TicksBothSides);
    barGraphTurnSlider->setPageStep(1);
    barGraphTurnSlider->setSingleStep(1);
    barGraphTurnSlider->setRange(0,1);
    barGraphTurnSlider->setVisible(false);
    connect(barGraphTurnSlider,SIGNAL(valueChanged(int)),this,SLOT(barGraphTurnSliderChanged(int)));

    //positioning widgets in the grid
    barGraphGridLayout->addWidget(barGraphTurnSlider,0,0,1,1,Qt::AlignTop);
    barGraphGridLayout->addWidget(barGraphControlsFrame,1,1,Qt::AlignRight);
}

void MainWindow::initializeBarGraphPlot()
{
    QFont font("Helvetica[Adobe]",10);
    barGraphTitle = new QCPPlotTitle(barCustomGraph," ");
    barGraphTitle->setFont(font);
    barGraphTitle->setTextColor(QColor(51,51,255));

    barCustomGraph->plotLayout()->insertRow(0);
    barCustomGraph->plotLayout()->addElement(0, 0, barGraphTitle);

    barCustomGraph->xAxis->setRange(0, 100);
    barCustomGraph->xAxis->setAutoTicks(true);
    barCustomGraph->xAxis->setAutoTickLabels(true);
    barCustomGraph->xAxis->setAutoTickStep(false);
    barCustomGraph->xAxis->setTickStep(10.0);

    barCustomGraph->xAxis->grid()->setVisible(true);
    barCustomGraph->setMinimumWidth(250);
    barCustomGraph->xAxis->setLabel("Position");

    barCustomGraph->yAxis->setAutoTicks(true);
    barCustomGraph->yAxis->setAutoTickLabels(true);
    barCustomGraph->yAxis->setRange(0, 100);
    barCustomGraph->yAxis->setPadding(5); // a bit more space to the left border
    barCustomGraph->yAxis->setLabel("Effective Power\nInfluence×Salience");
    barCustomGraph->yAxis->grid()->setSubGridVisible(false);

    connect( barCustomGraph->xAxis, SIGNAL(rangeChanged(QCPRange,QCPRange)), this, SLOT(xAxisRangeChanged(QCPRange,QCPRange)) );
    connect( barCustomGraph->yAxis, SIGNAL(rangeChanged(QCPRange,QCPRange)), this, SLOT(yAxisRangeChanged(QCPRange,QCPRange)) );

    // setup legend:
    barCustomGraph->legend->setVisible(false);
    barCustomGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                    QCP::iSelectLegend | QCP::iSelectPlottables);

    // setup policy and connect slot for context menu popup:
    barCustomGraph->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(barCustomGraph, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(barPlotContextMenuRequest(QPoint)));

}

void MainWindow::xAxisRangeChanged( const QCPRange &newRange, const QCPRange &oldRange )
{
    if( newRange.lower < 0 )
    {
        barCustomGraph->xAxis->setAutoTicks(false);
        barCustomGraph->xAxis->setAutoTickLabels(false);
        // barCustomGraph->xAxis->setRangeLower( 0 );
        // barCustomGraph->xAxis->setRangeUpper( oldRange.upper );
    }
    else
    {
        barCustomGraph->xAxis->setAutoTicks(true);
        barCustomGraph->xAxis->setAutoTickLabels(true);
        //         barCustomGraph->xAxis->setTicks(true);
        //        barCustomGraph->xAxis->setRangeUpper( newRange.upper );
    }
}
void MainWindow::yAxisRangeChanged( const QCPRange &newRange, const QCPRange &oldRange )
{
    if( newRange.lower < 0 )
    {
        barCustomGraph->yAxis->setAutoTicks(false);
        barCustomGraph->yAxis->setAutoTickLabels(false);
        //        barCustomGraph->yAxis->setTicks(false);
        //        barCustomGraph->yAxis->setRangeLower(0 );
        //        barCustomGraph->yAxis->setRangeUpper( oldRange.upper );
    }
    else
    {
        barCustomGraph->yAxis->setAutoTicks(true);
        barCustomGraph->yAxis->setAutoTickLabels(true);
        //         barCustomGraph->yAxis->setTicks(true);
        //        barCustomGraph->yAxis->setRangeUpper( newRange.upper );
    }
}

void MainWindow::barPlotContextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);

    menu->addAction("Save As BMP", this, SLOT(saveBarPlotAsBMP()));
    menu->addAction("Save As PDF", this, SLOT(saveBarPlotAsPDF()));

    menu->popup(barCustomGraph->mapToGlobal(pos));
}

void MainWindow::saveBarPlotAsBMP()
{
    QString fileName = getImageFileName("BMP File (*.bmp)","BarPlot",".bmp");
    if(!fileName.isEmpty())
    {
        barCustomGraph->saveBmp(fileName);
        //        setCurrentFile(fileName);
    }
}

void MainWindow::saveBarPlotAsPDF()
{
    QString fileName = getImageFileName("PDF File (*.pdf)","BarPlot",".pdf");
    if(!fileName.isEmpty())
    {
        barCustomGraph->savePdf(fileName);
        //        setCurrentFile(fileName);
    }
}

void MainWindow::populateBarGraphActorsList()
{
    QCheckBox * actor;
    barGraphActorsCheckBoxList.clear();
    barGraphCheckedActorsIdList.clear();
    barActorCBList.clear();

    QWidget* widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);

    for(int actorsCount = 0; actorsCount < actorsName.count(); ++actorsCount)
    {
        actor = new QCheckBox(actorsName.at(actorsCount));
        actor->setChecked(true);

        QColor mycolor = colorsList.at(actorsCount);

        QString style = "background: rgb(%1, %2, %3);";
        style = style.arg(mycolor.red()).arg(mycolor.green()).arg(mycolor.blue());
        style += "color:white; font-size:15px;";
        style += "font-weight:bold;";

        actor->setStyleSheet(style);

        actor->setObjectName(QString::number(actorsCount));

        layout->addWidget(actor);
        layout->stretch(0);

        barGraphActorsCheckBoxList.append(actor);

        //setting all checkboxes as checked as initial condition
        barGraphCheckedActorsIdList.append(true);
        connect(actor,SIGNAL(toggled(bool)),this,SLOT(barGraphActorsCheckboxClicked(bool)));
        barActorCBList.append(actor);
    }
    barGraphActorsScrollArea->setWidget(widget);
}

void MainWindow::populateBarGraphDimensions(int dim)
{
    Q_UNUSED(dim)

    disconnect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    barGraphDimensionComboBox->clear();
    barGraphDimensionComboBox->addItem(" ");
    for(int dims = 0; dims < dimensionList.length(); ++ dims )
    {
        QString dim = dimensionList.at(dims);
        barGraphDimensionComboBox->addItem(dim.remove("\n"));
        barGraphDimensionComboBox->setItemData(dims+1,"Positions for Dimension "+ QString::number(dims+1),Qt::ToolTipRole);
    }
    connect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    barGraphDimensionComboBox->removeItem(0);
}

void MainWindow::populateBarGraphStateRange(int states)
{
    barGraphTurnSlider->setRange(0,states);
    connect(turnSlider,SIGNAL(valueChanged(int)),barGraphTurnSlider,SLOT(setValue(int)));
}

void MainWindow::generateColors()
{
    colorsList.clear();
    colorsList << QColor(255,130,0)<< QColor(220,20,60)<< QColor(255,215,0)<< QColor(175,238,238)<<QColor(0,0,205)
               << QColor(165,42,42)<<QColor(34,139,34)<<QColor(218,165,32)<< QColor (95,158,160)
               << QColor(199,21,133)<<QColor(205,50,205)<<QColor(245,205,250)<<QColor(0,206,209)
               << QColor(173,255,47)<<QColor(127,255,212)<<QColor(65,105,225)<<QColor(255,20,147)
               << QColor(255,140,00)<<QColor(154,205,50)<< QColor(0,255,255)<<QColor(0,139,139)
               << QColor(64,224,208)<<QColor(255,0,255)<< QColor(255,228,196)<<QColor(188,143,143)
               << QColor(138,43,226)<<QColor(75,0,130)<< QColor(123,104,238)<<QColor(255,105,180)
               << QColor(70,130,180)<<QColor(205,103,63)<<QColor(255,99,71)<<QColor(0,255,0)
               << QColor(184,134,11)<<QColor(139,0,139)<<QColor(70,130,180)<<QColor(255,127,80)
               << QColor(152,251,152)<<QColor(0,191,255)<<QColor(153,50,204)<<QColor(186,85,211)
               << QColor(216,191,216)<<QColor(255,182,193)<<QColor(244,164,96)<<QColor(176,196,222)
               << QColor(240,128,128)<<QColor(250,128,114)<<QColor(0,250,154)<<QColor(30,144,255)
               << QColor(210,105,30)<<QColor(46,139,87)<<QColor(135,206,250)<<QColor(221,160,221)
               << QColor(250,235,215)<<QColor(222,184,135)<<QColor(240,255,240)<<QColor(255,69,0)
               << QColor(32,178,170)<<QColor(255,140,00)<<QColor(255,140,0)<<QColor(25,25,112)
               << QColor(124,252,0)<<QColor(155,255,127)<<QColor(176,224,230)<<QColor(238,232,170)
               << QColor(240,230,140)<<QColor(128,128,0)<< QColor(255,255,0);

    for(int g = 67; g<=255; g++)
        colorsList.append(QColor(rand()%255,rand()%255,rand()%255));
}

void MainWindow::getActorsInRange(int dim)
{
    deleteBars();

    //calculating bin_width
    double numberOfBins = barGraphGroupRangeLineEdit->text().toDouble();
    double lowerRange = 0.0;
    double upperRange = 1.0;

    binWidth = (upperRange-lowerRange)/numberOfBins;

    double r1 =0.0;
    double r2=binWidth;

    while(r2 <=1.0)
    {
        emit getActorIdsInRange(r1*100,r2*100,dim,barGraphTurnSlider->value());
        r1=r2;
        r2+=binWidth;
    }
}

void MainWindow::deleteBars()
{
    for(int i= 0; i < 100; ++i)
    {
        if(!bars[i].isEmpty())
        {
            qDeleteAll(bars[i].begin(),bars[i].end());
            bars[i].clear();
        }
    }

    barsCount=0;
}

QCPBars *MainWindow::createBar(int actorId)
{
    QCPBars *bar = new QCPBars(barCustomGraph->xAxis, barCustomGraph->yAxis);
    QPen pen;
    pen.setWidthF(1.2);
    pen.setColor(QColor(105,105,105));
    bar->setWidth(binWidth*100);
    bar->setPen(pen);
    bar->setBrush(colorsList.at(actorId));
    //    bar->setBrush(colorsList.at (in));
    bar->setName("actorId");
    bar->setObjectName(QString::number(actorId));
    bar->setSelectable(false);

    //    QPen pen1;
    //    pen1.setColor(QColor(0,0,0));
    //    bar->setSelectedBrush(QColor(211,211,211,30));
    //    bar->setSelectedPen(pen1);

    return bar;
}

void MainWindow::barGraphSelectAllActorsCheckBoxClicked(bool Click)
{
    for(int index=0; index < barActorCBList.length();++ index)
        disconnect(barActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(barGraphActorsCheckboxClicked(bool)));

    for(int actors = 0 ; actors < barGraphCheckedActorsIdList.length(); ++actors)
    {
        barGraphCheckedActorsIdList[actors]=Click;
        barGraphActorsCheckBoxList[actors]->setChecked(Click);
    }
    for(int index=0; index < barActorCBList.length();++ index)
        connect(barActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(barGraphActorsCheckboxClicked(bool)));

    barGraphTurnSliderChanged(barGraphTurnSlider->value());
}

//actors clicked slot
void MainWindow::barGraphActorsCheckboxClicked(bool click)
{
    QCheckBox * actorCheckBox = qobject_cast<QCheckBox *>(sender());
    quint8 actorId = actorCheckBox->objectName().toInt();

    barGraphCheckedActorsIdList[actorId]=click;

    barGraphTurnSliderChanged(barGraphTurnSlider->value());
}

void MainWindow::barGraphDimensionChanged(int value)
{
    dimension=value;
    lineGraphDimensionComboBox->setCurrentIndex(value);
    barGraphTitle->setText(QString(barGraphDimensionComboBox->currentText() +"-Effective Power Landscape, Turn " +QString::number(barGraphTurnSlider->value())));
    getActorsInRange(dimension);
    barCustomGraph->xAxis->setRange(0,100);
    barCustomGraph->yAxis->setRange(0,yAxisLen+20);
    yAxisLen=50;
    barCustomGraph->replot();
}

void MainWindow::barGraphTurnSliderChanged(int value)
{
    barGraphTitle->setText(QString(barGraphDimensionComboBox->currentText() +"-Effective Power Landscape, Turn " +QString::number(value)));
    getActorsInRange(dimension);
    barCustomGraph->xAxis->setRange(0,100);
    barCustomGraph->yAxis->setRange(0,yAxisLen+20);
    yAxisLen=50;
    barCustomGraph->replot();
}

void MainWindow::barGraphBinWidthButtonClicked(bool bl)
{
    Q_UNUSED(bl)

    barGraphTurnSliderChanged(barGraphTurnSlider->value());
}

void MainWindow::barGraphActorsSalienceCapability(QVector<int> aId, QVector<double> sal, QVector<double> cap,double r1,double r2)
{

    if( barActorCBList.length() == lineActorCBList.length())
    {
        if(!aId.isEmpty() && !barGraphCheckedActorsIdList.isEmpty())
        {
            actorsIdsClr.clear();
            QVector <double> range;
            QVector <double> stackedActorsSelected[100];
            QVector <double> stackedActorsUnselected[100];

            QCPBars * bar;
            double barHeight = 0;

            QVector <QVector <double> > values ;
            range<< ((r1+r2)/2);

            for (int stackedActorsInOneBar = 0 ; stackedActorsInOneBar < aId.length(); ++ stackedActorsInOneBar)
            {
                if(barGraphCheckedActorsIdList.at(aId.at(stackedActorsInOneBar))==true)
                {
                    stackedActorsSelected[stackedActorsInOneBar] << (sal.at(stackedActorsInOneBar) * cap.at(stackedActorsInOneBar));
                    values.append(stackedActorsSelected[stackedActorsInOneBar]);
                    actorsIdsClr.append(aId.at(stackedActorsInOneBar));
                    barHeight += (sal.at(stackedActorsInOneBar) * cap.at(stackedActorsInOneBar));
                }
            }

            int sel = values.length();

            for (int stackedActorsInOneBar = 0 ; stackedActorsInOneBar < aId.length(); ++ stackedActorsInOneBar)
            {
                if(barGraphCheckedActorsIdList.at(aId.at(stackedActorsInOneBar))==false)
                {
                    stackedActorsUnselected[stackedActorsInOneBar] << (sal.at(stackedActorsInOneBar) * cap.at(stackedActorsInOneBar));
                    values.append(stackedActorsUnselected[stackedActorsInOneBar]);
                    barHeight += (sal.at(stackedActorsInOneBar) * cap.at(stackedActorsInOneBar));
                }
            }

            for (int i =0; i < values.length(); ++i)
            {
                if(i < sel)
                {
                    prevBar= bar;
                    bar = createBar(actorsIdsClr.at(i));
                    bar->setData(range,values.at(i));

                    if(i>0)
                        bar->moveAbove(prevBar);

                    bars[barsCount].append(bar);
                }
                else
                {
                    prevBar= bar;
                    bar = createBar(aId.at(i));
                    bar->setBrush(QColor(211,211,211,70));

                    bar->setData(range,values.at(i));

                    if(i>=sel && i >0)
                        bar->moveAbove(prevBar);

                    bars[barsCount].append(bar);
                }
            }

            if(barsCount < 100)
                ++barsCount;
            else
                barsCount=0;

            if(barHeight>yAxisLen)
            {
                yAxisLen=barHeight;
                barHeight=0;
            }
        }
    }
}

void MainWindow::updateBarDimension(QStringList *dims)
{
    QStringList  dimenList;
    for(int index=0;index<dims->length();++index)
    {
        dimenList.append(dims->at(index));
    }
    int barDimIndex = barGraphDimensionComboBox->currentIndex();

    disconnect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    barGraphDimensionComboBox->clear();
    for(int dims = 0; dims < dimenList.length(); ++ dims )
    {
        barGraphDimensionComboBox->addItem(dimenList.at(dims));
        barGraphDimensionComboBox->setItemData(dims+1,"Positions for Dimension "+ QString::number(dims+1),Qt::ToolTipRole);

    }
    connect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    barGraphDimensionComboBox->currentIndexChanged(barDimIndex);
}

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
