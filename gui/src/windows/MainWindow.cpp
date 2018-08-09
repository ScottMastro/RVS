#include "MainWindow.h"
#include "PlotWindow.h"

#include "ui_mainwindow.h"
#include <iostream>
#include "../log/qlog.h"
#include "../src/vikNGS.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":icon.svg"));

    //required to push log updates to textbox
    connect(getQLog(), SIGNAL(pushOutput(QString, QColor)), this, SLOT(printOutput(QString, QColor)));
    qRegisterMetaType<Data>("Result");
    qRegisterMetaType<std::vector<std::vector<Variant>>>("std::vector<std::vector<Variant>>");
    qRegisterMetaType<SimulationRequest>("SimulationRequest");

    simulationTabInit();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::printOutput(QString str, QColor c){

    QString oldOut = ui->outputBox->toHtml();
    oldOut = oldOut.right(50000);
    int close = oldOut.indexOf("<");
    oldOut = oldOut.right(50000 - close);

    QString newOut = "<font color=\"" + c.name() + "\">" + str + "</font>";

    //avoid having the first line blank
    if(ui->outputBox->toPlainText().size() < 2)
        ui->outputBox->setHtml(newOut);
    else
        ui->outputBox->setHtml(oldOut + newOut);

    ui->outputBox->verticalScrollBar()->setValue(ui->outputBox->verticalScrollBar()->maximum());
    ui->outputBox->repaint();
}

void MainWindow::greyOutput(){

    //return if nothing to grey
    if(ui->outputBox->toPlainText().size() < 2)
        return;

    QString oldOut = ui->outputBox->toHtml();
    oldOut = oldOut.replace(QRegExp("color:#.{6};"), "color:" + grey.name() + ";");

    ui->outputBox->setHtml(oldOut + "<br>");
    ui->outputBox->verticalScrollBar()->setValue(ui->outputBox->verticalScrollBar()->maximum());
    ui->outputBox->repaint();
}

void MainWindow::stopJob(){
    STOP_RUNNING_THREAD=true;
    while(!jobThread->isFinished()){
        jobThread->quit();
    }
    greyOutput();
    printInfo("Job stopped.");
    enableRun();
    STOP_RUNNING_THREAD=false;
}

void MainWindow::enableRun(){
    ui->main_runBtn->setEnabled(true);
    ui->main_stopBtn->setEnabled(false);
    ui->qsim_runBtn->setEnabled(true);
    ui->qsim_stopBtn->setEnabled(false);
    ui->sim_runBtn->setEnabled(true);
    ui->sim_stopBtn->setEnabled(false);
}
void MainWindow::disableRun(){
    ui->main_runBtn->setEnabled(false);
    ui->main_stopBtn->setEnabled(true);
    ui->qsim_runBtn->setEnabled(false);
    ui->qsim_stopBtn->setEnabled(true);
    ui->sim_runBtn->setEnabled(false);
    ui->sim_stopBtn->setEnabled(true);
}

void MainWindow::jobFinished(Data result){
    ui->main_runBtn->setEnabled(true);

    if(result.size() > 0){
        PlotWindow *plotter = new PlotWindow();
        QString title = "Plot " + QString::number(plotCount);
        plotCount++;
        plotter->initialize(result, title);
        printOutput("Displaying results in " + title.toLower(), green);
        plotter->show();
    }
}
