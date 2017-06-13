#include "databasedialog.h"
#include "ui_databasedialog.h"
#include <QDebug>

DatabaseDialog::DatabaseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DatabaseDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("KTAB_SMP:Configure Database");

    connect(ui->nextPushButton, SIGNAL(clicked(bool)),this,SLOT(nextClicked(bool)));
    ui->label_5->setVisible(false);
    ui->postgreDBLineEdit->setVisible(false);

}

DatabaseDialog::~DatabaseDialog()
{
    delete ui;
}

bool DatabaseDialog::showDialog()
{
    this->show();
    ui->stackedWidget->setCurrentIndex(0);
    return false;
}

void DatabaseDialog::nextClicked(bool)
{
    if(ui->sqlComboBox->currentIndex()==1)
    {
        ui->stackedWidget->setCurrentIndex(ui->sqlComboBox->currentIndex()+1);
    }
    else if(ui->sqlComboBox->currentIndex()==0)
    {
        on_sqliteDonePushButton_clicked();
    }
}

void DatabaseDialog::on_sqliteDonePushButton_clicked()
{
    QString connectionSring;
    connectionSring.append("Driver=QSQLITE;");//connectionType
    connectionSring.append("Server=localhost;");//host address
    connectionSring.append("Database=").append("None").append(";");

    this->close();
    emit connectionStringPath(connectionSring);

}

void DatabaseDialog::on_postgrePushButton_clicked()
{
    bool status = true;
    QString connectionSring;
    connectionSring.append("Driver=QPSQL;");//connectionType

    connectionSring.append("Database=").append("postgres").append(";");

    if(!ui->usernameLineEdit->text().trimmed().isEmpty())
    {
        connectionSring.append("Uid=").append(ui->usernameLineEdit->text()).append(";");
    }
    else
    {
        status=false;
        QMessageBox::warning(this,"Database Configuration","Please enter the Username "); // username
    }

    if(!ui->passwordLineEdit->text().trimmed().isEmpty())
    {
        connectionSring.append("Pwd=").append(ui->passwordLineEdit->text()).append(";"); // password
    }
    else
    {
        status=false;
        QMessageBox::warning(this,"Database Configuration","Please enter the Password ");
    }

    if(!ui->ipLineEdit->text().trimmed().isEmpty())
    {
        connectionSring.append("Server=").append(ui->ipLineEdit->text()).append(";");  //host address
    }
    else
    {
        status=false;
        QMessageBox::warning(this,"Database Configuration","Please fill Database Server Address");
    }
    if(!ui->portLineEdit->text().trimmed().isEmpty())
    {
        connectionSring.append("Port=").append(ui->portLineEdit->text()).append(";");  //port address, optional
    }

    if(status==true)
    {
        this->close();
        emit connectionStringPath(connectionSring);
    }
}

void DatabaseDialog::on_pushButton_clicked()
{
    //    QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    //    QString name (UTC.toString());
    //    name.replace(" ","_").replace(":","_");

    //    QFileDialog fileDialog(this);
    //    dbFilePath = fileDialog.getSaveFileName(this, tr("Save DB file as "),
    //                                            QString(/*homeDirectory+*/QDir::separator()+name),tr("DB File (*.db)"),
    //                                            0,QFileDialog::DontConfirmOverwrite);

    //    ui->sqliteDBLineEdit->setText(dbFilePath);
}

void DatabaseDialog::on_sqlComboBox_currentIndexChanged(int index)
{
    if(index==0)
    {
        ui->nextPushButton->setText("DONE");
    }
    else if (index == 1)
    {
        ui->nextPushButton->setText("NEXT");
    }
}
