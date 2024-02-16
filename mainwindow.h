#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QNetworkInterface>
#include <QTextStream>
#include <QFile>

#define HAVE_REMOTE

#include <pcap.h>
#include <pcap-bpf.h>
#include <pcap-stdinc.h>

#include "hal_thread.h"
#include <signal.h>
#include <stdio.h>
#include "sv_subscriber.h"
#include "qcustomplot.h"




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ClientThread : public QThread
{
    Q_OBJECT

signals:
    void updateUI(const QString msg);
    void updatePlot(const QVector<float>& vetor);
    void checkStatusThread(const int x);

public:
    explicit ClientThread(QObject *parent = nullptr);
    void stopThread();
    void run() override;

private slots:
    static void svUpdateListener(SVSubscriber subscriber, void* parameter, SVSubscriber_ASDU asdu);

public:
    bool running;
    QString eht;
    int lossPacket = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onUpdateUI(const QString msg);
    void onUpdatePlot(const QVector<float>& vetor);
    void onStatusCheck(const int x);

private slots:
    void salvarTexto(const QString &texto);
    void updatePlot();

    void on_actionCheck_Devices_triggered();
    void on_starButton_clicked();

public:
    Ui::MainWindow *ui;
    ClientThread* clientThread = nullptr;
    QString eht = "0";
    QTimer *plotTimer;    // Timer para atualização do gráfico
    QVector<float> data[8] = {
        QVector<float>(), //IA
        QVector<float>(), //IB
        QVector<float>(), //IC
        QVector<float>(), //IN
        QVector<float>(), //VA
        QVector<float>(), //VB
        QVector<float>(), //VC
        QVector<float>()  //VN
    };

};
#endif // MAINWINDOW_H
