#ifndef DATABASEDIALOG_H
#define DATABASEDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>

namespace Ui {
class DatabaseDialog;
}

class DatabaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseDialog(QWidget *parent = 0);
    ~DatabaseDialog();
    bool showDialog();

private:
    Ui::DatabaseDialog *ui;
    QString dbFilePath;
signals:
    void connectionStringPath(QString conStr);


private slots :
    void nextClicked(bool);

    void on_sqliteDonePushButton_clicked();
    void on_postgrePushButton_clicked();
    void on_pushButton_clicked();
    void on_sqlComboBox_currentIndexChanged(int index);
};

#endif // DATABASEDIALOG_H
