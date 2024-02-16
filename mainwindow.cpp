#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    plotTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Configuração inicial do QCustomPlot
    // Certifique-se de ter o QCustomPlot adicionado ao seu projeto
    QPen customPen1(QColor("blue")); // blue line
    customPen1.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen2(QColor("red")); // red line
    customPen2.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)
    QPen customPen3(QColor("green")); // green line
    customPen2.setWidth(2); // Define a largura da linha como 2 pixels (ou o valor desejado)

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


    // Conecte o timer para atualizar o gráfico
    connect(plotTimer, &QTimer::timeout, this, &MainWindow::updatePlot);

    // Restaurar valor do arquivo .ini no início do programa
    QSettings settings("myapp.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    if (settings.contains("Interface")){
        eht = settings.value("Interface", "").toString();
    }else{
        eht = "0";
    }
    settings.endGroup();

    ui->lineEdit->setText(eht);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::salvarTexto);

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

    QSettings settings("myapp.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("Interface", eht);
    settings.endGroup();

    delete ui;
    //   delete serverThread;
}
void MainWindow::on_starButton_clicked()
{
    if(ui->starButton->text() == "START"){
        clientThread = new ClientThread(this);
        // Conecte o sinal updateUI à slot onUpdateUI
        connect(clientThread, &ClientThread::updateUI, this, &MainWindow::onUpdateUI);
        connect(clientThread, &ClientThread::updatePlot, this, &MainWindow::onUpdatePlot);
        connect(clientThread, &ClientThread::checkStatusThread, this, &MainWindow::onStatusCheck);

        clientThread->eht = eht;
        clientThread->start();
        // Inicie o timer para atualizar o gráfico a cada intervalo
        plotTimer->start(50);  // Ajuste conforme necessário
        ui->starButton->setText("STOP");
    }else{
        //disconnect(clientThread, nullptr, nullptr, nullptr);
        clientThread->stopThread();
        clientThread->wait();
        delete clientThread;
        clientThread = nullptr;
        plotTimer->stop();
        ui->starButton->setText("START");
    }

}
void MainWindow::salvarTexto(const QString &texto)
{
    eht = texto;
}
void MainWindow::onUpdateUI(const QString msg)
{
    ui->textEdit->append(msg);
}
void MainWindow::onStatusCheck(const int x)
{
    if(x == 0){
        ui->starButton->setText("START");
        plotTimer->stop();
    }
}
void MainWindow::onUpdatePlot(const QVector<float>& vetor)
{
    data[0].append(vetor[0]);
    data[1].append(vetor[1]);
    data[2].append(vetor[2]);
    data[3].append(vetor[3]);
    data[4].append(vetor[4]);
    data[5].append(vetor[5]);
    data[6].append(vetor[6]);
    data[7].append(vetor[7]);
}
void MainWindow::updatePlot()
{
    QTime horaAtual = QTime::currentTime();
    double key = horaAtual.msecsSinceStartOfDay()/1000.0; // time elapsed since start of demo, in seconds
    static int len = 0;
    int increment;
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

    len = size;
}
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

ClientThread::ClientThread(QObject *parent)
    : QThread(parent)
{
    // Inicialize membros ou faça outras configurações necessárias
}
void ClientThread::stopThread() {
    requestInterruption();  // Solicita a interrupção da thread
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
        while (!isInterruptionRequested())
        {
            QThread::sleep(1);

            if (lossPacket > 0){
                emit updateUI(QString("Numero de pacotes perdidos no ultimo segundo: %1").arg(lossPacket));
                lossPacket = 0;
            }
        }
    }
    else {
        emit updateUI("Failed to start SV subscriber. "
                             "Reason can be that the Ethernet"
                             " interface doesn't exist or root"
                             " permissions are required.");
        emit checkStatusThread(0);
    }
    emit updateUI("Encerrando a Thread...");

    SVReceiver_stop(receiver);
    SVReceiver_removeSubscriber(receiver, subscriber);
    SVReceiver_destroy(receiver);

    emit updateUI("Thread encerrada!");
    QThread::msleep(100);
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

    if (SVSubscriber_ASDU_getDataSize(asdu) >= 32) {
        vetor[0] = SVSubscriber_ASDU_getFLOAT32(asdu, 0);
        vetor[1] = SVSubscriber_ASDU_getFLOAT32(asdu, 4);
        vetor[2] = SVSubscriber_ASDU_getFLOAT32(asdu, 8);
        vetor[3] = SVSubscriber_ASDU_getFLOAT32(asdu, 12);
        vetor[4] = SVSubscriber_ASDU_getFLOAT32(asdu, 16);
        vetor[5] = SVSubscriber_ASDU_getFLOAT32(asdu, 20);
        vetor[6] = SVSubscriber_ASDU_getFLOAT32(asdu, 24);
        vetor[7] = SVSubscriber_ASDU_getFLOAT32(asdu, 28);

        emit instance->updatePlot(vetor);
    }
}











