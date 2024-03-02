#include "mainwindow.h"
#include "ui_mainwindow.h"

/////////////////////////////////////////////////////////////////////////////
/// Variaveis globais
///
struct myPyStruct{
    int len;
    float stlIn[4096];
    float stlT[4096];
    float stlS[4096];
    float stlR[4096];
};
myPyStruct *structPointerPy;

TCHAR MapObj_py[]=TEXT("Value_Mapping_py");
TCHAR MapObj_WorkEvent_py[]=TEXT("WorkEvent_py");
TCHAR MapObj_BackEvent_py[]=TEXT("BackEvent_py");

HANDLE hMapFile_py;
HANDLE workHandle_py;
HANDLE backHandle_py;

/////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), plotTimer(new QTimer(this))
{
    ui->setupUi(this);

    graficSetting();

    restoreData();

    ui->lineEdit->setText(eht);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::salvarTexto);

    QList<int> sizes;
    sizes << 100;   // Tamanho inicial para o widget da esquerda (0 ou qualquer valor desejado)
    sizes << 900; // Tamanho inicial para o widget da direita
    ui->splitter_2->setSizes(sizes);
    ui->splitter_3->setSizes(sizes);

    ui->tabWidget->setCurrentIndex(0);

    // Obtém a paleta de cores atual
    QPalette pal = ui->lcd_freq->palette();
    pal.setColor(QPalette::WindowText, Qt::red);
    pal.setColor(QPalette::Window, Qt::black);
    ui->lcdNumber->setPalette(pal);
    ui->lcd_freq->setPalette(pal);
    ui->lcdAmostral->setPalette(pal);

    // Conecte o timer para atualizar o gráfico
    plotTimer = new QTimer(this);
    plotTimer_m = new QTimer(this);
    fftTimer = new QTimer(this);

    connect(plotTimer, &QTimer::timeout, this, &MainWindow::updatePlot);

    data.resize(cN);
    for (int i = 0; i < cN; ++i) {
        data[i].reserve(5000000);  // Reserva espaço dentro de cada vetor interno
    }
    dataTime.reserve(5000000);

}
MainWindow::~MainWindow()
{
    if (clientThread) {
        disconnect(clientThread, nullptr, nullptr, nullptr);
        clientThread->running = false;
        clientThread->quit();
        clientThread->wait();
        delete clientThread;
        clientThread = nullptr;
    }

    // Salvando o estado atual da janela
    QSettings settings("myapp.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("Interface", eht);
    settings.endGroup();

    delete ui;
    //   delete serverThread;
}
void MainWindow::restoreData()
{
    // Configurações do arquivo myapp.ini
    QCoreApplication::setOrganizationName("AWI");
    QCoreApplication::setApplicationName("Pnrf ATP");

    // Restaurando o estado anterior da janela
    QSettings settings("myapp.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");

    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const QByteArray state = settings.value("windowState", QByteArray()).toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    if (settings.contains("Interface")){
        eht = settings.value("Interface", QString()).toString();
    }else{
        eht = "3";
    }
    settings.endGroup();
}
void MainWindow::graficSetting()
{
    // Configuração inicial do QCustomPlot
    // Certifique-se de ter o QCustomPlot adicionado ao seu projeto
    QPen customPen1(QColor("blue")); // blue line
    customPen1.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen2(QColor("red")); // red line
    customPen2.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen3(QColor("green")); // green line
    customPen3.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen4(QColor("Yellow")); // red line
    customPen4.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen5(QColor("Gray")); // green line
    customPen5.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen6(QColor("Black")); // green line
    customPen6.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)

    ui->customPlot->addGraph(); // Gráfico 1
    ui->customPlot->addGraph(); // Gráfico 2
    ui->customPlot->addGraph(); // Gráfico 3
    // Configurar as propriedades dos gráficos
    ui->customPlot->graph(0)->setPen(customPen1); // Configurar as propriedades do Gráfico 1
    ui->customPlot->graph(1)->setPen(customPen2); // Configurar as propriedades do Gráfico 2
    ui->customPlot->graph(2)->setPen(customPen3); // Configurar as propriedades do Gráfico 3
    ui->customPlot->xAxis->setLabel("Tempo");
    ui->customPlot->yAxis->setLabel("Tensão");
    ui->customPlot->yAxis->setRange(-1, 1);  // Ajuste conforme necessário

    ui->customPlot1->addGraph(); // Gráfico 1
    ui->customPlot1->addGraph(); // Gráfico 2
    ui->customPlot1->addGraph(); // Gráfico 3
    // Configurar as propriedades dos gráficos
    ui->customPlot1->graph(0)->setPen(customPen1); // Configurar as propriedades do Gráfico 1
    ui->customPlot1->graph(1)->setPen(customPen2); // Configurar as propriedades do Gráfico 2
    ui->customPlot1->graph(2)->setPen(customPen3); // Configurar as propriedades do Gráfico 3
    ui->customPlot1->xAxis->setLabel("Tempo");
    ui->customPlot1->yAxis->setLabel("Corrente");
    ui->customPlot1->yAxis->setRange(-1, 1);  // Ajuste conforme necessário

    ui->customPlot2->addGraph(); // Gráfico 1
    ui->customPlot2->graph(0)->setPen(customPen1);
    ui->customPlot2->xAxis->setLabel("Tempo");
    ui->customPlot2->yAxis->setLabel("IN");
    ui->customPlot2->yAxis->setRange(-1, 1);  // Ajuste conforme necessário

    ///// grafico 2 Metodo

    ui->customPlot_m->addGraph(); // Gráfico 1
    ui->customPlot_m->addGraph(); // Gráfico 2
    ui->customPlot_m->addGraph(); // Gráfico 3
    // Configurar as propriedades dos gráficos
    ui->customPlot_m->graph(0)->setPen(customPen1); // Configurar as propriedades do Gráfico 1
    ui->customPlot_m->graph(1)->setPen(customPen2); // Configurar as propriedades do Gráfico 2
    ui->customPlot_m->graph(2)->setPen(customPen3); // Configurar as propriedades do Gráfico 3
    ui->customPlot_m->xAxis->setLabel("Tempo");
    ui->customPlot_m->yAxis->setLabel("Tensão");
    ui->customPlot_m->yAxis->setRange(-1, 1);  // Ajuste conforme necessário

    ui->customPlot1_m->addGraph(); // Gráfico 1
    ui->customPlot1_m->addGraph(); // Gráfico 2
    ui->customPlot1_m->addGraph(); // Gráfico 3
    // Configurar as propriedades dos gráficos
    ui->customPlot1_m->graph(0)->setPen(customPen1); // Configurar as propriedades do Gráfico 1
    ui->customPlot1_m->graph(1)->setPen(customPen2); // Configurar as propriedades do Gráfico 2
    ui->customPlot1_m->graph(2)->setPen(customPen3); // Configurar as propriedades do Gráfico 3
    ui->customPlot1_m->xAxis->setLabel("Tempo");
    ui->customPlot1_m->yAxis->setLabel("Corrente");
    ui->customPlot1_m->yAxis->setRange(-1, 1);  // Ajuste conforme necessário

    ui->customPlot2_m->addGraph(); // Gráfico 1
    ui->customPlot2_m->addGraph(); // Gráfico 1
    ui->customPlot2_m->addGraph(); // Gráfico 1
    ui->customPlot2_m->addGraph(); // Gráfico 1
    ui->customPlot2_m->addGraph(); // Gráfico 1
    ui->customPlot2_m->addGraph(); // Gráfico 1
    ui->customPlot2_m->graph(0)->setPen(customPen1);
    ui->customPlot2_m->graph(1)->setPen(customPen2);
    ui->customPlot2_m->graph(2)->setPen(customPen3);
    ui->customPlot2_m->graph(3)->setPen(customPen4);
    ui->customPlot2_m->graph(4)->setPen(customPen5);
    ui->customPlot2_m->graph(5)->setPen(customPen6);
    ui->customPlot2_m->xAxis->setLabel("Tempo");
    ui->customPlot2_m->yAxis->setLabel("IN");
    ui->customPlot2_m->yAxis->setRange(-1, 1);  // Ajuste conforme necessário
}
/////////////////////////////////////////////////////////////////////////////
/// \brief MainWindow::creatPyConection
///
void MainWindow::creatPyConection()
{
    hMapFile_py = CreateFileMapping(  INVALID_HANDLE_VALUE,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,
                                      sizeof(myPyStruct),
                                      MapObj_py);
    if (hMapFile_py == NULL) return ui->textEdit->append("Erro ao criar hMapFile_py");

    structPointerPy = (myPyStruct*)MapViewOfFile(   hMapFile_py,
                                                    FILE_MAP_ALL_ACCESS,
                                                    0,
                                                    0,
                                                    sizeof(myPyStruct));
    if (structPointerPy == NULL) return ui->textEdit->append("Erro ao abrir structPointerPy");

    workHandle_py = CreateEvent(NULL, TRUE, FALSE, MapObj_WorkEvent_py); // Manually. no signal
    backHandle_py = CreateEvent(NULL, TRUE, FALSE, MapObj_BackEvent_py); // Manually. no signal

    if (workHandle_py == NULL || backHandle_py == NULL) return ui->textEdit->append("Erro no CreateEvent");

    // Especifica o script Python a ser executado
    QString scriptPath = "C:/Users/Aldair/GoogleDrive/Doutorado/PROJETOS/pyProjects/auxPyForDocPro/main.py";

    // Define o vetor que você deseja passar para o script
    QStringList arguments;
    arguments.append(scriptPath);

    process = new QProcess(this);
    // Conectar o sinal finished ao slot onProcessFinished
    connect(process, &QProcess::finished, this, &MainWindow::onProcessFinished);

    // Inicia o processo Python
    process->start("python", arguments);

    if (process->waitForStarted()) {
        // O processo foi iniciado com sucesso
    } else {
        // O processo não pôde ser iniciado, exibe uma mensagem de erro
        QByteArray error = process->readAllStandardError();
        qDebug() << "Erro ao iniciar o processo:" << error;
    }
}
/////////////////////////////////////////////////////////////////////////////
/// \brief Thread Handle
///
void MainWindow::onThreadDestroyed(QObject *obj){
    if (obj == clientThread) {
        ui->starButton->setText("Iniciar o Monitoramento");
    }
    else if (obj == myMethod) {
        ui->iniMetButon->setText("Inicializar Metodo");
        // disconnect(myMethod, nullptr, nullptr, nullptr);
        // delete myMethod;
        // myMethod = nullptr;
    }
}
void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug() << "Processo finalizado com código de saída:" << exitCode;
    qDebug() << "Status de saída:" << exitStatus;
    QByteArray result = process->readAllStandardOutput();
    qDebug() << "Saidas: " << result;
}
/////////////////////////////////////////////////////////////////////////////
/// \brief Button and stuffs
///
void MainWindow::on_starButton_clicked()
{
    if(ui->starButton->text() == "Iniciar o Monitoramento"){
        clientThread = new ClientThread(this);
        // Conecte o sinal updateUI à slot onUpdateUI
        connect(clientThread, &ClientThread::finished, clientThread, &ClientThread::deleteLater);
        connect(clientThread, &ClientThread::destroyed, this, &MainWindow::onThreadDestroyed);
        connect(clientThread, &ClientThread::updateUI, this, &MainWindow::onUpdateUI);
        connect(clientThread, &ClientThread::updateData, this, &MainWindow::onUpdateData);

        clientThread->eht = eht;
        clientThread->start();
        // Inicie o timer para atualizar o gráfico a cada intervalo
        plotTimer->start(83);  // Ajuste conforme necessário
        ui->starButton->setText("Parar o Monitoramento");
    }else{
        plotTimer->stop();
        clientThread->requestInterruption();
    }
}
void MainWindow::on_iniMetButon_clicked()
{
    if(ui->iniMetButon->text() == "Inicializar Metodo"){
        myMethod = new MyMethod(this, &data, &dataTime, &lastDataIndex, 1, 80, 60, Va);
        // Conecte o sinal updateUI à slot onUpdateUI
        connect(myMethod, &MyMethod::finished, myMethod, &MyMethod::deleteLater);
        connect(myMethod, &MyMethod::destroyed, this, &MainWindow::onThreadDestroyed);
        connect(myMethod, &MyMethod::updateUI, this, &MainWindow::onUpdateUI);

        connect(plotTimer_m, &QTimer::timeout, this, &MainWindow::updatePlot_m);

        connect(fftTimer, &QTimer::timeout, myMethod, &MyMethod::onFftTimer);

        if(clientThread != nullptr)
        {
            fftTimer->start(8);
            plotTimer_m->start(200);
        }
        else ui->textEdit->append("Monitoramento do SV não esta habilitado!\n");

        myMethod->start();

        ui->iniMetButon->setText("Parar o Metodo");

    }else{
        disconnect(fftTimer, &QTimer::timeout, myMethod, &MyMethod::onFftTimer);
        plotTimer_m->stop();
        fftTimer->stop();
        myMethod->requestInterruption();
    }
}
void MainWindow::on_spButon_clicked()
{
    creatPyConection();
}
void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 1)
    {
        plotTimer->stop();
    }
    if (index == 0 && clientThread != nullptr)
    {
        plotTimer->start(50);
    }
}
/////////////////////////////////////////////////////////////////////////////
/// \brief MainWindow::onUpdateUI
///
void MainWindow::onUpdateUI(const QString msg)
{
    ui->textEdit->append(msg);
}
void MainWindow::onUpdateData(const QVector<float> &vetor, const Timestamp &tempo)
{
    data[0].append(vetor[0]);
    data[1].append(vetor[1]);
    data[2].append(vetor[2]);
    data[3].append(vetor[3]);
    data[4].append(vetor[4]);
    data[5].append(vetor[5]);
    data[6].append(vetor[6]);
    data[7].append(vetor[7]);

    quint64 t = Timestamp_getTimeInNs(const_cast<Timestamp*>(&tempo));
    dataTime.append(t);

    lastDataIndex = data[7].size() - 1;
    /// calcula a quantas amostras tem em um ciclo
    /// ref a tensao na fase A
    static float vaLast = 0.1;
    static qint64 zCrosLast = 0;
    if(vetor[4] > 0 && vaLast <= 0)
    {
        SRate = data[4].size() - zCrosLast;
        freq = 1.0/((dataTime.last() - dataTime.at(zCrosLast - 1)) * 1e-9);
        zCrosLast = data[4].size();
        stepTime = (dataTime.last()-dataTime.at(dataTime.size() - 2)) * 1e-9;
        //qDebug() << stepTime;
    }
    vaLast = vetor[4];
}
void MainWindow::updatePlot()
{
    static int len = 0;
    int increment = 1;
    int size = 0;
    static float maxG1 = 0;
    static float maxG2 = 0;
    static float maxG3 = 0;
    static float minG1 = 0;
    static float minG2 = 0;
    static float minG3 = 0;

    if(!data[7].isEmpty())
    {
        size = data[7].size();
        increment = size - len;
        if (increment>0){
            for(int i=len; i<size; i++)
            {
                if(data[4].at(i) > maxG1) maxG1 = data[4].at(i);
                if(data[5].at(i) > maxG1) maxG1 = data[5].at(i);
                if(data[6].at(i) > maxG1) maxG1 = data[6].at(i);

                if(data[0].at(i) > maxG2) maxG2 = data[0].at(i);
                if(data[1].at(i) > maxG2) maxG2 = data[1].at(i);
                if(data[2].at(i) > maxG2) maxG2 = data[2].at(i);

                if(data[3].at(i) > maxG3) maxG3 = data[3].at(i);

                if(data[4].at(i) < minG1) minG1 = data[4].at(i);
                if(data[5].at(i) < minG1) minG1 = data[5].at(i);
                if(data[6].at(i) < minG1) minG1 = data[6].at(i);

                if(data[0].at(i) < minG2) minG2 = data[0].at(i);
                if(data[1].at(i) < minG2) minG2 = data[1].at(i);
                if(data[2].at(i) < minG2) minG2 = data[2].at(i);

                if(data[3].at(i) < minG3) minG3 = data[3].at(i);

                ui->customPlot->graph(0)->addData(i, data[4].at(i));
                ui->customPlot->graph(1)->addData(i, data[5].at(i));
                ui->customPlot->graph(2)->addData(i, data[6].at(i));

                ui->customPlot1->graph(0)->addData(i, data[0].at(i));
                ui->customPlot1->graph(1)->addData(i, data[1].at(i));
                ui->customPlot1->graph(2)->addData(i, data[2].at(i));

                ui->customPlot2->graph(0)->addData(i, data[3].at(i));
            }
            // Rescale e replot
            ui->customPlot->yAxis->setRange(minG1, maxG1);
            ui->customPlot->xAxis->setRange(len, 500, Qt::AlignRight);
            ui->customPlot1->yAxis->setRange(minG2, maxG2);
            ui->customPlot1->xAxis->setRange(len, 500, Qt::AlignRight);
            ui->customPlot2->yAxis->setRange(minG3, maxG3);
            ui->customPlot2->xAxis->setRange(len, 500, Qt::AlignRight);

            ui->customPlot->replot(); //V
            ui->customPlot1->replot(); //I
            ui->customPlot2->replot(); //IN
        }
    }

    len = size;

    ui->lcdNumber->display(SRate);
    ui->lcd_freq->display(freq);
    ui->lcdAmostral->display(stepTime);
}
void MainWindow::updatePlot_m()
{
    static int len = 0;
    static float maxG1 = 0;
    static float maxG2 = 0;
    static float maxG3 = 0;
    static float minG1 = 0;
    static float minG2 = 0;
    static float minG3 = 0;
    int increment = 1;
    int size = 0;

    if(!myMethod->fftData[In][harm3].isEmpty())
    {
        size = myMethod->fftData[In][harm3].size();
        increment = size - len;
        if (increment > 0){
            for(int i=len; i<size; i++)
            {
                if(myMethod->fftData[Ia][harm1].at(i) > maxG1) maxG1 = myMethod->fftData[Ia][harm1].at(i);
                if(myMethod->fftData[Ib][harm1].at(i) > maxG1) maxG1 = myMethod->fftData[Ib][harm1].at(i);
                if(myMethod->fftData[Ic][harm1].at(i) > maxG1) maxG1 = myMethod->fftData[Ic][harm1].at(i);

                if(myMethod->fftData[In][harm1].at(i) > maxG2) maxG2 = myMethod->fftData[In][harm1].at(i);
                if(myMethod->fftData[In][harm3].at(i) > maxG2) maxG2 = myMethod->fftData[In][harm3].at(i);
                if(myMethod->fftData[In][harm5].at(i) > maxG2) maxG2 = myMethod->fftData[In][harm5].at(i);

                if(myMethod->fftData[In][harm3].at(i) > maxG3) maxG3 = myMethod->fftData[In][harm3].at(i);
                if(myMethod->bbv[h3h].at(i).BB_U > maxG3) maxG3 = myMethod->bbv[h3h].at(i).BB_U;
                if(myMethod->bbv[h3h].at(i).BB_D > maxG3) maxG3 = myMethod->bbv[h3h].at(i).BB_D;
                if(myMethod->bbv[h3h].at(i).SMA_longa > maxG3) maxG3 = myMethod->bbv[h3h].at(i).SMA_longa;
                if(myMethod->bbv[h3h].at(i).SMA_curta > maxG3) maxG3 = myMethod->bbv[h3h].at(i).SMA_curta;

                ui->customPlot_m->graph(0)->addData(i, myMethod->fftData[Ia][harm1].at(i));
                ui->customPlot_m->graph(1)->addData(i, myMethod->fftData[Ib][harm1].at(i));
                ui->customPlot_m->graph(2)->addData(i, myMethod->fftData[Ic][harm1].at(i));

                ui->customPlot1_m->graph(0)->addData(i, myMethod->fftData[In][harm1].at(i));
                ui->customPlot1_m->graph(1)->addData(i, myMethod->fftData[In][harm3].at(i));
                ui->customPlot1_m->graph(2)->addData(i, myMethod->fftData[In][harm5].at(i));

                ui->customPlot2_m->graph(0)->addData(i, myMethod->fftData[In][harm3].at(i));
                ui->customPlot2_m->graph(1)->addData(i, myMethod->bbv[h3h].at(i).BB_U);
                ui->customPlot2_m->graph(2)->addData(i, myMethod->bbv[h3h].at(i).BB_D);
                ui->customPlot2_m->graph(3)->addData(i, myMethod->bbv[h3h].at(i).SMA_longa);
                ui->customPlot2_m->graph(4)->addData(i, myMethod->bbv[h3h].at(i).SMA_curta);
                //ui->customPlot2_m->graph(5)->addData(len, fg);
            }

        // Rescale e replot
        ui->customPlot_m->yAxis->setRange(0, maxG1);
        ui->customPlot_m->xAxis->setRange(len, 1000, Qt::AlignRight);
        ui->customPlot1_m->yAxis->setRange(0, maxG2);
        ui->customPlot1_m->xAxis->setRange(len, 1000, Qt::AlignRight);
        ui->customPlot2_m->yAxis->setRange(0, maxG3);
        ui->customPlot2_m->xAxis->setRange(len, 1000, Qt::AlignRight);

        ui->customPlot_m->replot(); //V
        ui->customPlot1_m->replot(); //I
        ui->customPlot2_m->replot(); //IN
        }

    }

    len = size;
}
/////////////////////////////////////////////////////////////////////////////
/// \brief Othres
///
void MainWindow::on_actionCheck_Devices_triggered()
{
    pcap_if_t *devices;
    pcap_if_t *device;
    char errbuf[PCAP_ERRBUF_SIZE];
    char source[] = "rpcap://";

    /* Get the ethernet device list */
    if (pcap_findalldevs_ex(source, NULL, &devices, errbuf) == -1)
    {
        onUpdateUI(QString("pcap_findalldevs_ex: %1").arg(errbuf));
        return;
    }
    int index = 0;
    /* Iterate over the device list */
    for (device = devices; device != NULL; device = device->next)
    {
        // Use 'device' to access information about the current interface
        onUpdateUI(QString("Interface Name: %1").arg(device->name));
        onUpdateUI(QString("Interface Description: %1").arg(device->description));
        onUpdateUI(QString("Interface Index: %1").arg(index));

        onUpdateUI("");  // Adiciona uma linha em branco
        index++;
    }
    pcap_freealldevs(devices);
}
void MainWindow::salvarTexto(const QString &texto)
{
    eht = texto;
}
/////////////////////////////////////////////////////////////////////////////
/// \brief ClientThread::ClientThread
/// \param parent
///
ClientThread::ClientThread(QObject *parent) : QThread(parent)
{
    // Inicialize membros ou faça outras configurações necessárias
}
void ClientThread::run()
{
    SVReceiver receiver = SVReceiver_create();

    emit updateUI("Receiver created successfully.");
    emit updateUI("Thread iniciada");
    emit updateUI("Using interface: " + eht);

    //SVReceiver_disableDestAddrCheck(receiver);
    SVReceiver_setInterfaceId(receiver, eht.toUtf8().constData());
    /* Create a subscriber listening to SV messages with APPID 4000h */
    SVSubscriber subscriber = SVSubscriber_create(NULL, 0x4000);
    /* Install a callback handler for the subscriber */
    SVSubscriber_setListener(subscriber, svUpdateListener, this);
    /* Connect the subscriber to the receiver */
    SVReceiver_addSubscriber(receiver, subscriber);
    /* Start listening to SV messages - starts a new receiver background thread */
    SVReceiver_start(receiver);

    if (SVReceiver_isRunning(receiver)) {
        emit updateUI("Receiver is running and listening for messages.");
        int count1s = 0;
        while (!isInterruptionRequested())
        {
            QThread::msleep(200);

            if (count1s >= 5){
                if (lossPacket > 0){
                    emit updateUI(QString("Numero de pacotes perdidos no ultimo segundo: %1").arg(lossPacket));
                    lossPacket = 0;
                }
                count1s = 0;
            }
            count1s++;
        }
    }
    else {
        emit updateUI("Failed to start SV subscriber. "
                             "Reason can be that the Ethernet"
                             " interface doesn't exist or root"
                             " permissions are required.");
    }
    emit updateUI("Encerrando a Thread...");

    SVReceiver_stop(receiver);
    SVReceiver_removeSubscriber(receiver, subscriber);
    SVReceiver_destroy(receiver);

    emit updateUI("Thread encerrada!");
    QThread::msleep(10);
}
void ClientThread::svUpdateListener(SVSubscriber subscriber, void *parameter, SVSubscriber_ASDU asdu)
{
    static bool ini = true;
    static int lastcount = 0;
    int count = 0;

    ClientThread *instance = static_cast<ClientThread*>(parameter);
    QVector<float> vetor;
    vetor.resize(8);
    if (ini)
    {
        ini = false;
        const char* svID = SVSubscriber_ASDU_getSvId(asdu);

        if (svID != NULL) emit instance->updateUI(QString("svID=(%1)\n").arg(svID));

        emit instance->updateUI(QString("smpCnt: %1").arg(SVSubscriber_ASDU_getSmpCnt(asdu)));
        emit instance->updateUI(QString("confRev: %1").arg(SVSubscriber_ASDU_getConfRev(asdu)));
        //emit instance->updateUI(QString("Sample Rate: %1\n").arg(SVSubscriber_ASDU_getSmpRate(asdu)));
        //emit instance->updateUI(QString("Size in Bytes: %1\n").arg(SVSubscriber_ASDU_getDataSize(asdu)));
    }
    count = SVSubscriber_ASDU_getConfRev(asdu);
    if (count > lastcount + 1){
        instance->lossPacket = instance->lossPacket + count - (lastcount + 1);
    }
    lastcount = count;

    if (SVSubscriber_ASDU_getDataSize(asdu) >= 40) {
        vetor[0] = SVSubscriber_ASDU_getFLOAT32(asdu, 0);
        vetor[1] = SVSubscriber_ASDU_getFLOAT32(asdu, 4);
        vetor[2] = SVSubscriber_ASDU_getFLOAT32(asdu, 8);
        vetor[3] = SVSubscriber_ASDU_getFLOAT32(asdu, 12);
        vetor[4] = SVSubscriber_ASDU_getFLOAT32(asdu, 16);
        vetor[5] = SVSubscriber_ASDU_getFLOAT32(asdu, 20);
        vetor[6] = SVSubscriber_ASDU_getFLOAT32(asdu, 24);
        vetor[7] = SVSubscriber_ASDU_getFLOAT32(asdu, 28);

        Timestamp tempo = SVSubscriber_ASDU_getTimestamp(asdu, 32);

        emit instance->updateData(vetor, tempo);
    }
}
/////////////////////////////////////////////////////////////////////////////
/// \brief MyMethod::MyMethod
/// \param parent
///
MyMethod::MyMethod(QObject *parent, QVector<QVector<float>> *dados, QVector<quint64> *dadosTempo,
                             quint64 *lastIndex, int fftLW, int srate, float freq, int faseRef)
    : QThread(parent), data(dados), dataTime(dadosTempo), lastDataIndex(lastIndex)
{
    K.FFTLenWindow = fftLW;
    K.amostragem = srate;
    K.frequency = freq;
    K.phRef = faseRef;
    K.P_1h = 120;
    K.P_3h = 120;
    K.BB_w1h = 60;
    K.BB_w3h = 10;
    K.SMA_curta_w1h = 20;
    K.SMA_curta_w3h = 5;

    bbv[h1h].resize(121);
    bbv[h3h].resize(121);
    for (int i = 0; i < 121; ++i) {
        bbv[h1h][i] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        bbv[h3h][i] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    }

    Buildup.resize(1);
    porcentagem.resize(121);

    // Agora, redimensiona cada vetor interno para FFTLenWindow * amostragem elementos.
    for (int i = 0; i < 2; ++i) {
        angulo_M[i].append(0);
        modulo_M[i].append(0);
        angulo_predict[i].append(0);
        modulo_predict[i].append(0);
    }
    for (int i = 0; i < hN; i++){
        Cfc[i].resize(K.FFTLenWindow*K.amostragem);
        Cfs[i].resize(K.FFTLenWindow*K.amostragem);
    }
    K.K1 = 2.0*K.FFTLenWindow*3.14159265359/(K.FFTLenWindow*K.amostragem);
    K.K2 = 2.0/(K.amostragem*K.FFTLenWindow);
    for(int j=0;j<hN;j++){
        for(int kk=0;kk<K.FFTLenWindow*K.amostragem;kk++){
            float  angle = K.K1*j*kk;
            Cfc[j][kk] = cosf(angle);
            Cfs[j][kk] = sinf(angle);
        }
    }


    harmList << 1 << 3 << 5;
    chList << Ia << Ib << Ic << In << Va << Vb << Vc << Vn;

    flags.f1h=false;
    flags.f3h=false;
    flags.Var_M_flag_1h = false;
    flags.Var_B_flag_1h = false;
    flags.Var_End_flag_1h = false;
    flags.Var_End_flag_3h = false;
    flags.Save_buildup_flag = false;
    flags.Var_BD_flag_3h = false;
    flags.Var_B_flag_3h = false;
    flags.flag_capturar_3h = false;
    flags.var_derivada = false;
    flags.var_estavel_direcao_3h = false;
    flags.var_estavel_amplitude_3h = false;
    flags.var_abaixo_amplitude_3h = false;

    count.Var_BU_count_1h = 0;
    count.Var_MM_count_1h = 0;
    count.count_break_1h = 0;
    count.flag_1h_position = 0;
    count.count_break_1h = 0;
    count.Var_BU_count_3h = 0;
    count.Position_flag_3h = 0;
    count.count_BB_abaixo_amplitude = 0;

}
void MyMethod::onFftTimer()
{
    static quint64 ifft = 0;
    quint64 dataIndex = *lastDataIndex;

    //-------- Calculo do FFT ----------

    //qDebug() << data[Ia].at(dataIndex);

    for (int &harmValue : harmList) {
        if (harmValue >= hN) continue;
        for (int &chValue : chList) {
            if (chValue >= cN) continue;
            QVector<float> fftresult = fft_complete(chValue,
                                                    harmValue,
                                                    dataIndex);
            fftModulo[chValue][harmValue] = fftresult[0];
            fftAngulo[chValue][harmValue] = fftresult[1];

        }
    }


    for (int &harmValue : harmList) {
        if (harmValue >= hN) continue;
        for (int &chValue : chList) {
            if (chValue >= cN) continue;
            phaseAngulo[chValue][harmValue] = phase_biuld(harmValue,
                                                          fftAngulo[K.phRef][harm1],
                                                          fftAngulo[chValue][harmValue]);
        }
    }
    for (int i = 0; i < cN; ++i) {
        for (int j = 0; j < hN; ++j) {
            fftData[i][j].append(fftModulo[i][j]);
            phaseData[i][j].append(phaseAngulo[i][j]);
        }
    }

    /////////////////////////////////////

    //-------- Fasor Medio ----------

    if (flags.f1h == 1 || flags.f3h == 1) // for harm1
    {
        angulo_M[h1h].append(angulo_M[h1h].last());
        modulo_M[h1h].append(modulo_M[h1h].last());
    }else{
        QVector<float> result = mediadofasor(fftModulo[In][harm1],
                                             phaseAngulo[In][harm1],
                                             modulo_M[h1h].last(),
                                             angulo_M[h1h].last(),
                                             K.P_1h);
        angulo_M[h1h].append(result[0]);
        modulo_M[h1h].append(result[1]);
    }

    if (flags.f3h == 1) // for harm3
    {
        angulo_M[h3h].append(angulo_M[h3h].last());
        modulo_M[h3h].append(modulo_M[h3h].last());
    }else{
        QVector<float> result = mediadofasor(fftModulo[In][harm3],
                                             phaseAngulo[In][harm3],
                                             modulo_M[h3h].last(),
                                             angulo_M[h3h].last(),
                                             K.P_3h);
        angulo_M[h3h].append(result[0]);
        modulo_M[h3h].append(result[1]);
    }

    //------- Fasor Predict ---------------------

    QVector<float> result1h = subtraifasor(fftModulo[In][harm1],
                                           phaseAngulo[In][harm1],
                                           modulo_M[h1h].last(),
                                           angulo_M[h1h].last());
    QVector<float> result3h = subtraifasor(fftModulo[In][harm3],
                                           phaseAngulo[In][harm3],
                                           modulo_M[h3h].last(),
                                           angulo_M[h3h].last());

    angulo_predict[h1h].append(result1h[0]);
    modulo_predict[h1h].append(result1h[1]);

    angulo_predict[h3h].append(result3h[0]);
    modulo_predict[h3h].append(result3h[1]);

    //------- Angulo referencial

    float ang_ref = angulo_predict[h3h].last() - 3*angulo_predict[h1h].last();

    while(ang_ref >= 360){
        ang_ref = ang_ref - 360;
    }
    while(ang_ref < 0){
        ang_ref = ang_ref + 360;
    }
    Angulo_3h_ref1h.append(ang_ref);

    //-------- Calculo de Medias e Bandas de Boillinger 1h ------------------


    if (ifft >= 120)
    {
        //----------------Calcula medias-------------------

        float SMA_longa_aux_1h = 0;
        for (int j = ifft;j > ifft-K.BB_w1h+1; j--){
            SMA_longa_aux_1h = SMA_longa_aux_1h + modulo_predict[h1h].at(j);
        }
        bbp[h1h].SMA_longa = SMA_longa_aux_1h/K.BB_w1h;

        float SMA_curta_aux_1h = 0;
        for (int j = ifft; j > ifft-K.SMA_curta_w1h+1; j--){
            SMA_curta_aux_1h = SMA_curta_aux_1h + modulo_predict[h1h].at(j);
        }
        bbp[h1h].SMA_curta = SMA_curta_aux_1h/K.SMA_curta_w1h;

        float aux = 0;
        for (int j = ifft;j > ifft-K.BB_w1h; j--){
            aux = aux + qPow(modulo_predict[h1h].at(j) - bbp[h1h].SMA_longa, 2);
        }
        bbp[h1h].BB_DP = sqrt(aux/(K.BB_w1h-1));

        bbp[h1h].BB_U = bbp[h1h].SMA_longa + 2*bbp[h1h].BB_DP;
        bbp[h1h].BB_D = bbp[h1h].SMA_curta - 2*bbp[h1h].BB_DP;

        bbv[h1h].append(bbp[h1h]);

        //-------------Logica--------------------

        if (bbp[h1h].SMA_curta > bbp[h1h].SMA_longa && bbp[h1h].SMA_curta >= 0.01){
            count.Var_MM_count_1h = count.Var_MM_count_1h + 1;
            if (count.Var_MM_count_1h > 60){
                flags.Var_M_flag_1h = true;
                Buildup_temp.append(bbp[h1h].SMA_longa);
            }
        }else{
            if (flags.Var_M_flag_1h == true && flags.Save_buildup_flag == true){
                Buildup = Buildup_temp;
                if (Buildup.size() > 10){
                    flags.Save_buildup_flag = false;
                }
            }
            flags.Var_M_flag_1h = false;
            count.Var_MM_count_1h = 0;
        }

        if (modulo_predict[h1h].last() > bbp[h1h].BB_U && modulo_predict[h1h].last() >= 0.01){
            count.Var_BU_count_1h = count.Var_BU_count_1h + 1;
            if (count.Var_BU_count_1h > 3){
                flags.Var_B_flag_1h = true;
            }
        }else{
            count.Var_BU_count_1h = 0;
            flags.Var_B_flag_1h = false;
        }

        if ((flags.Var_B_flag_1h == true || flags.Var_M_flag_1h == true) && flags.f1h == false){
            flags.f1h = true;
            count.flag_1h_position = ifft;
            BB_U_1h_prev = bbv[h1h].at(ifft-K.BB_w1h).BB_U;
            BB_D_1h_prev = bbv[h1h].at(ifft-K.BB_w1h).BB_D;
        }

        //flag_1h_V(i) = flag_1h;


        if (flags.f1h == true){

            float var_estabilidade_1h = ((bbp[h1h].BB_U - bbp[h1h].BB_D) - (bbv[h1h].at(ifft-K.BB_w1h).BB_U
                                          - bbv[h1h].at(ifft-K.BB_w1h).BB_D)) / (bbp[h1h].BB_U-bbp[h1h].BB_D);

            float var_test_1h = ((bbp[h1h].BB_U-bbp[h1h].BB_D) - (BB_U_1h_prev-BB_D_1h_prev) ) / (bbp[h1h].BB_U-bbp[h1h].BB_D);
            float var_test2_1h = abs(1-(bbp[h1h].SMA_longa/bbv[h1h].at(ifft-K.BB_w1h).SMA_longa));

            if(var_estabilidade_1h < 0.2 && var_test_1h < 0.5 && var_test2_1h < 0.1){
                count.count_break_1h = count.count_break_1h + 1;
                if (count.count_break_1h >= 10  && bbp[h1h].SMA_curta < bbp[h1h].SMA_longa){
                    flags.Var_End_flag_1h = true;
                    //                     flag_1h = 0;
                }
            }else{
                count.count_break_1h = 0;
            }
            //Var_End_flagM_1h(i) = Var_End_flag_1h;
        }else{
            //Var_End_flagM_1h(i) = 0;
        }

    }


    //-------- Calculo de Medias e Bandas de Boillinger 3h ------------------


    if (ifft > 120)
    {
        static float SMA_curta_V_3hLast = 0;

        static int BB_U_min_prev_3h = 0;
        static int BB_U_max_prev_3h = 0;
        static int BB_D_min_prev_3h = 0;
        static int BB_D_max_prev_3h = 0;
        static int SMA_longa_min_prev_3h = 0;
        static int SMA_longa_max_prev_3h = 0;
        static int BB_U_min = 0;
        static int BB_U_max = 0;
        static int BB_D_min = 0;
        static int BB_D_max = 0;
        static int SMA_longa_min = 0;
        static int SMA_longa_max = 0;

        //----------------Calcula medias-------------------

        float SMA_longa_aux_3h = 0;
        for (int j = ifft;j > ifft-K.BB_w3h+1; j--){
            SMA_longa_aux_3h = SMA_longa_aux_3h + modulo_predict[h3h].at(j);
        }
        bbp[h3h].SMA_longa = SMA_longa_aux_3h/K.BB_w3h;

        float SMA_curta_aux_3h = 0;
        for (int j = ifft;j > ifft-K.SMA_curta_w3h+1; j--){
            SMA_curta_aux_3h = SMA_curta_aux_3h + modulo_predict[h3h].at(j);
        }
        bbp[h3h].SMA_curta = SMA_curta_aux_3h/K.SMA_curta_w3h;

        X_Der_M_curta3h = bbp[h3h].SMA_curta - SMA_curta_V_3hLast;
        SMA_curta_V_3hLast = bbp[h3h].SMA_curta;

        float aux = 0;
        for (int j = ifft;j > ifft-K.BB_w3h; j--){
            aux = aux + qPow(modulo_predict[h3h].at(j) - bbp[h3h].SMA_longa, 2);
        }
        bbp[h3h].BB_DP = sqrt(aux/(K.BB_w3h-1));

        bbp[h3h].BB_U = bbp[h3h].SMA_longa + 2*bbp[h3h].BB_DP;
        bbp[h3h].BB_D = bbp[h3h].SMA_longa - 2*bbp[h3h].BB_DP;

        bbv[h3h].append(bbp[h3h]);

        float BB_soma = 0;
        for (int j = ifft;j > ifft-60+1; j--){
            BB_soma = BB_soma + bbv[h3h].at(j).BB_DP;
        }
        BB_sdp_Media.append(BB_soma/60);

        //------------------------------------------

        if (bbp[h3h].BB_D > ( bbp[h3h].BB_U-bbp[h3h].BB_D ) && bbp[h3h].SMA_curta >= 0.01 ){
            flags.Var_BD_flag_3h = true;
            //Var_BD_flag_3_V(i) = 0.8;
        }else{
            flags.Var_BD_flag_3h = false;
        }

        if (modulo_predict[harm3].last() > bbp[h3h].BB_U && bbp[h3h].SMA_curta >= 0.01){
            count.Var_BU_count_3h = count.Var_BU_count_3h + 1;
            if (count.Var_BU_count_3h > 3){
                flags.Var_B_flag_3h = true;
                //Var_B_flag_3h_V(i) = 0.9;
            }
        }else{
            count.Var_BU_count_3h = 0;
            flags.Var_B_flag_3h = 0;
        }

        if ((flags.Var_B_flag_3h || flags.Var_BD_flag_3h) && flags.f3h == false && ifft > 180 && flags.flag_capturar_3h == false){
            flags.f3h = true;
            flags.flag_capturar_3h = true;
            count.Position_flag_3h = ifft;
            count.count_BB_abaixo_amplitude = 0;

            BB_U_min_prev_3h = bbp[h3h].BB_U;
            BB_U_max_prev_3h = bbp[h3h].BB_U;
            BB_D_min_prev_3h = bbp[h3h].BB_D;
            BB_D_max_prev_3h = bbp[h3h].BB_D;
            SMA_longa_min_prev_3h = bbp[h3h].SMA_longa;
            SMA_longa_max_prev_3h = bbp[h3h].SMA_longa;

            for (int j = ifft; j > ifft-60; j--){
                if (BB_U_min_prev_3h > bbv[h3h].at(j).BB_U)
                    BB_U_min_prev_3h = bbv[h3h].at(j).BB_U;
                if (BB_U_max_prev_3h < bbv[h3h].at(j).BB_U)
                    BB_U_max_prev_3h = bbv[h3h].at(j).BB_U;
                if (BB_D_min_prev_3h > bbv[h3h].at(j).BB_D)
                    BB_D_min_prev_3h = bbv[h3h].at(j).BB_D;
                if (BB_D_max_prev_3h < bbv[h3h].at(j).BB_D)
                    BB_D_max_prev_3h = bbv[h3h].at(j).BB_D;
                if (SMA_longa_min_prev_3h > bbv[h3h].at(j).SMA_longa)
                    SMA_longa_min_prev_3h = bbv[h3h].at(j).SMA_longa;
                if (SMA_longa_max_prev_3h < bbv[h3h].at(j).SMA_longa)
                    SMA_longa_max_prev_3h = bbv[h3h].at(j).SMA_longa;
            }

        }

        //flag_3h_V(i) = flag_3h;
        //flag_efetivo_3h_V(i) = flag_3h;
        float tmax_flag_3h = 0;

        if (flags.f3h){
            tmax_flag_3h = tmax_flag_3h + 1;

            if (X_Der_M_curta3h > 0.1) flags.var_derivada = true;

            BB_U_min = bbp[h3h].BB_U;
            BB_U_max = bbp[h3h].BB_U;
            BB_D_min = bbp[h3h].BB_D;
            BB_D_max = bbp[h3h].BB_D;
            SMA_longa_min = bbp[h3h].SMA_longa;
            SMA_longa_max = bbp[h3h].SMA_longa;

            for (int j = ifft; j > ifft-150; j--){
                if (BB_U_min > bbv[h3h].at(j).BB_U)
                    BB_U_min = bbv[h3h].at(j).BB_U;
                if (BB_U_max < bbv[h3h].at(j).BB_U)
                    BB_U_max = bbv[h3h].at(j).BB_U;
                if (BB_D_min > bbv[h3h].at(j).BB_D)
                    BB_D_min = bbv[h3h].at(j).BB_D;
                if (BB_D_max < bbv[h3h].at(j).BB_D)
                    BB_D_max = bbv[h3h].at(j).BB_D;
                if (SMA_longa_min > bbv[h3h].at(j).SMA_longa)
                    SMA_longa_min = bbv[h3h].at(j).SMA_longa;
                if (SMA_longa_max < bbv[h3h].at(j).SMA_longa)
                    SMA_longa_max = bbv[h3h].at(j).SMA_longa;
            }

            if (SMA_longa_max < BB_U_min && SMA_longa_min > BB_D_max){
                flags.var_estavel_direcao_3h = true;
                //var_estavel_direcao_3h_V(i) = 1.2;
            }else{
                flags.var_estavel_direcao_3h = false;
            }

            if ( (BB_U_max-BB_D_min) < 2*(BB_U_max_prev_3h-BB_D_min_prev_3h) ){
                flags.var_estavel_amplitude_3h = true;
                //var_estavel_amplitude_3h_V(i) = 1.1;
            }else{
                flags.var_estavel_amplitude_3h = false;
            }

            if ( (BB_U_max-BB_D_min) < 0.005 ){
                flags.var_abaixo_amplitude_3h = true;
                //var_estavel_amplitude_3h_V(i) = 1.3;
            }else{
                flags.var_abaixo_amplitude_3h = false;
            }


            if( (flags.var_estavel_direcao_3h && flags.var_estavel_amplitude_3h) || flags.var_abaixo_amplitude_3h || tmax_flag_3h>1200 || flags.var_derivada ){
                flags.Var_End_flag_3h = true;
                //Var_End_flagM_3h(i) = 1;
                flags.f3h = false;
                // if (Position_flag_3h_end == FT_si){
                //     Position_flag_3h_end = i-10;
                // }
            }

        }else{
            //Var_End_flagM_3h(i) = 0;
        }
        //if (Var_End_flag_3h == 1) flag_efetivo_3h_V(i-10) = 0;

    }

    //-------- Calculo da porcentagem -------------------

    if (modulo_predict[h1h].last() > 0.1){
        porcentagem.append(100*modulo_predict[h3h].last()/modulo_predict[h1h].last());
        //ref_porc(i) = 3;
    }else{
        porcentagem.append(0);
        //ref_porc(i) = 3;
        //if (Var_End_flag_3h == 1) Porcentagem(i-11) = 0;
    }


    ifft++;

    //qDebug() << countSs;
}
void MyMethod::run()
{

    while(1)
    {
        if(isInterruptionRequested())
        {

            break;
        }
        QThread::msleep(200);
    }

}
/////////////////////////////////////////////////////////////////////////////
/// \brief
///
float MyMethod::phase_biuld(int harm, float fftAref, float fftA){
    float gallas;
    float angle_biuld;

    gallas = (fftA-90);
    if(gallas<=-180.0){
        gallas += 360;
    }
    angle_biuld = (fftAref-90);
    if(angle_biuld<=-180.0){
        angle_biuld += 360;
    }
    angle_biuld = angle_biuld * harm;
    while(angle_biuld>=180.0){
        angle_biuld -= 360;
    }
    while(angle_biuld<=-180.0){
        angle_biuld += 360;
    }
    gallas = gallas - angle_biuld;
    if(gallas>=180.0){
        gallas=gallas-360;
    }
    if(gallas<=-180.0){
        gallas=gallas+360;
    }
    if((gallas>=-0.0001)&&(gallas<=0.0001)){
        gallas = 0;
    }
    return gallas;
}
QVector<float> MyMethod::fft_complete(int ch, int harm, quint64 lastIndex){

    QVector<float> fft(2);
    float fftR = 0;
    float fftI = 0;

    lastIndex = lastIndex - (K.FFTLenWindow*K.amostragem);

    for(int i=0;i<(K.FFTLenWindow*K.amostragem);i++){
        fftR += (*data)[ch][lastIndex + i] * Cfc[harm][i];
        fftI += (*data)[ch][lastIndex + i] * Cfs[harm][i];
    }

    fftR = K.K2*fftR;
    fftI = K.K2*fftI;

    fft[0] = sqrt(fftR*fftR+(fftI*fftI));
    fft[1] = atan2(fftI,fftR)*57.295779;

    return fft;
}
QVector<float> MyMethod::pol2cart(float th, float r)
{
    QVector<float> conv_p(2);

    th = th*0.0174532925;

    conv_p[0] = r*cos(th);
    conv_p[1] = r*sin(th);

    return conv_p;
}
QVector<float> MyMethod::cart2pol(float x, float y)
{
    QVector<float> conv_c(2);

    conv_c[0] = atan2(y,x)*57.295779;
    conv_c[1] = hypot(x,y);

    return conv_c;
}
float MyMethod::abs(float x){
    if(x<0)
        x = x*-1;
    return x;
}
QVector<float> MyMethod::mediadofasor(float modulo, float angulo, float modulo_M, float angulo_M, int multiplicador){
    QVector<float> conv_p(2);
    QVector<float> temp_m(2);
    float x;
    float y;

    temp_m = pol2cart(angulo,modulo);
    x = temp_m[0];
    y = temp_m[1];
    temp_m = pol2cart(angulo_M,modulo_M*multiplicador);
    x = x+temp_m[0];
    y = y+temp_m[1];

    temp_m = cart2pol(x,y);

    conv_p[0] = temp_m[0];
    conv_p[1] = temp_m[1]/(multiplicador+1);

    return conv_p;
}
QVector<float> MyMethod::subtraifasor(float modulo1, float angulo1, float modulo2, float angulo2){
    QVector<float> conv_su(2);
    QVector<float> temp_m(2);
    float x;
    float y;

    temp_m = pol2cart(angulo1,modulo1);
    x = temp_m[0];
    y = temp_m[1];
    temp_m = pol2cart(angulo2,modulo2);
    x = x-temp_m[0];
    y = y-temp_m[1];

    temp_m = cart2pol(x,y);

    conv_su[0] = temp_m[0];
    conv_su[1] = temp_m[1];

    return conv_su;
}
float MyMethod::mediadafase(float phase, float faseM, float divisor){
    QVector<float> temp_m(2);
    float x;
    float y;
    float conv_m;

    temp_m = pol2cart(phase,1);
    x = temp_m[0];
    y = temp_m[1];
    temp_m = pol2cart(faseM,divisor-1);
    x = x+temp_m[0];
    y = y+temp_m[1];

    temp_m = cart2pol(x,y);

    conv_m = temp_m[0];

    return conv_m;
}





