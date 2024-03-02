#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QNetworkInterface>
#include <QElapsedTimer>
#include <QTextStream>
#include <QFile>
#include <QtMath>
#include <QProcess>

#define HAVE_REMOTE

#include <pcap.h>
#include <pcap-bpf.h>
#include <pcap-stdinc.h>

#include "sv_subscriber.h"

#define cN 8
#define hN 8

#define Ia 0
#define Ib 1
#define Ic 2
#define In 3
#define Va 4
#define Vb 5
#define Vc 6
#define Vn 7

#define harm0 0
#define harm1 1
#define harm2 2
#define harm3 3
#define harm4 4
#define harm5 5
#define harm6 6
#define harm7 7

#define h1h 0
#define h3h 1

struct BB{
    float BB_U;
    float BB_D;
    float BB_DP;
    float SMA_longa;
    float SMA_curta;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/////////////////////////////////////////////////////////////////////////////
/// \brief The MyMethod class
///
class MyMethod : public QThread
{
    Q_OBJECT

signals:
    void updateUI(const QString msg);
    void updatePlot();

public: //metodos publicos
    MyMethod(QObject *parent = nullptr, QVector<QVector<float>> *dados = nullptr,
             QVector<quint64> *dadosTempo = nullptr, quint64 *lastIndex = nullptr,
             int fftLW = 1, int srate = 80, float freq = 60, int faseRef = 4);

    void setVarList(QList<int> hrL, QList<int> chL){harmList = hrL; chList = chL;}

private: //funções privadas
    void run() override;
    QVector<float> pol2cart(float th, float r);
    QVector<float> cart2pol(float x, float y);
    float abs(float x);
    QVector<float> mediadofasor(float modulo, float angulo, float modulo_M, float angulo_M, int multiplicador);
    QVector<float> subtraifasor(float modulo1, float angulo1, float modulo2, float angulo2);
    float mediadafase(float phase, float faseM, float divisor);
    float phase_biuld(int harm, float fftAref, float fftA);
    QVector<float> fft_complete(int ch, int harm, quint64 lastIndex);

public slots:
    void onFftTimer();

private: //variaveis da classe
    QVector<float> Cfc[hN], Cfs[hN]; //matriz da FFT
    float fftModulo[cN][hN], fftAngulo[cN][hN], phaseAngulo[cN][hN]; //matriz instantanea
    float BB_U_1h_prev, BB_D_1h_prev, X_Der_M_curta3h;

    struct constante{
        int FFTLenWindow;
        int amostragem;
        int phRef;
        int P_1h, P_3h;
        int BB_w1h, BB_w3h;
        int SMA_curta_w1h, SMA_curta_w3h;
        float K1, K2, frequency;
    };
    constante K;

    BB bbp[2];

    QVector<QVector<float>> *data; //dados de tensao e corrente
    QVector<quint64> *dataTime; //respectivos tempos
    quint64 *lastDataIndex = 0; //ultimo indice valido para todos canais

public:
    QList<int> harmList, chList;
    QVector<float> fftData[cN][hN], phaseData[cN][hN]; //modulo e angulo vetor
    QVector<float> angulo_M[2], modulo_M[2], angulo_predict[2], modulo_predict[2];
    QVector<float> Buildup_temp, Buildup, Angulo_3h_ref1h, porcentagem, BB_sdp_Media;

    QVector<BB> bbv[2];

    struct flag{
        bool f1h;
        bool f3h;
        bool Var_M_flag_1h;
        bool Var_B_flag_1h;
        bool Var_End_flag_1h;
        bool Var_End_flag_3h;
        bool Save_buildup_flag;
        bool Var_BD_flag_3h;
        bool Var_B_flag_3h;
        bool flag_capturar_3h;
        bool var_derivada;
        bool var_estavel_direcao_3h;
        bool var_estavel_amplitude_3h;
        bool var_abaixo_amplitude_3h;
    };
    flag flags;

    struct contador{
        int Var_MM_count_1h;
        int Var_BU_count_1h ;
        int flag_1h_position;
        int count_break_1h;
        int Var_BU_count_3h;
        int Position_flag_3h;
        int count_BB_abaixo_amplitude;
    };
    contador count;
};

/////////////////////////////////////////////////////////////////////////////
/// \brief The ClientThread class
///
class ClientThread : public QThread
{
    Q_OBJECT
signals:
    void updateUI(const QString msg);
    void updateData(const QVector<float>& vetor, const Timestamp &tempo);
public:
    ClientThread(QObject *parent = nullptr);
    void run() override;
private slots:
    static void svUpdateListener(SVSubscriber subscriber, void* parameter, SVSubscriber_ASDU asdu);
public:
    bool running;
    QString eht;
    int lossPacket = 0;
};

/////////////////////////////////////////////////////////////////////////////
/// \brief The MainWindow class
///
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void restoreData();
    void graficSetting();

public slots:
    void onUpdateUI(const QString msg);
    void onUpdateData(const QVector<float>& vetor, const Timestamp &tempo);
    void updatePlot();
    void updatePlot_m();

private slots:
    void salvarTexto(const QString &texto);

    void onThreadDestroyed(QObject *obj);
    void onProcessFinished(int, QProcess::ExitStatus);
    void on_actionCheck_Devices_triggered();
    void on_starButton_clicked();
    void on_iniMetButon_clicked();
    void on_spButon_clicked();
    void on_tabWidget_currentChanged(int index);

private:
    void creatPyConection();

public:
    Ui::MainWindow *ui;
    ClientThread *clientThread = nullptr;
    MyMethod *myMethod = nullptr;
    QProcess *process;
    QString eht = "3";
    QTimer *plotTimer;    // Timer para atualização do gráfico
    QTimer *plotTimer_m;
    QTimer *fftTimer;

    QVector<QVector<float>> data; //dados de tensao e corrente
    QVector<quint64> dataTime; //respectivos tempos
    quint64 lastDataIndex = 0; //ultimo indice valido para todos canais

private:
    int SRate = 0;
    double freq = 0;
    double stepTime = 0;

};
#endif // MAINWINDOW_H
