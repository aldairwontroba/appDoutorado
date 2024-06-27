#include "mainwindow.h"
#include "ui_mainwindow.h"

/////////////////////////////////////////////////////////////////////////////
/// Variaveis globais
///
struct myPyStruct{
    int tipo;
    int lenpos;
    int lensig;
    float h1h_M;
    float h1a_M;
    float h3h_M;
    float h3a_M;
    float pyh1h[4096];
    float pyh3h[4096];
    float pyh3a[4096];
    float signal[40*4096];
};
myPyStruct *structPointerPy;

TCHAR MapObj_py[]=TEXT("Value_Mapping_py");
TCHAR MapObj_WorkEvent_py[]=TEXT("WorkEvent_py");
TCHAR MapObj_BackEvent_py[]=TEXT("BackEvent_py");

HANDLE hMapFile_py;
HANDLE workHandle_py;
HANDLE backHandle_py;

QString svidref = "svpub1";
int svReadtype = 0;
float svMultiply = 0.01;

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
    settings.setValue("svId", svidref);
    settings.setValue("svReadtype", svReadtype);
    settings.setValue("svMultiply", svMultiply);
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
    if (settings.contains("svId")){
        svidref = settings.value("svId", QString()).toString();
    }
    if (settings.contains("svReadtype")){
        svReadtype = settings.value("svReadtype", QByteArray()).toInt();
    }
    if (settings.contains("svMultiply")){
        svMultiply = settings.value("svMultiply", QByteArray()).toFloat();
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
    ui->customPlot2->addGraph(); // Gráfico 1
    ui->customPlot2->graph(0)->setPen(customPen1);
    ui->customPlot2->graph(1)->setPen(customPen2);
    ui->customPlot2->xAxis->setLabel("Tempo");
    ui->customPlot2->yAxis->setLabel("Corrente");
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
    ui->customPlot_m->yAxis->setLabel("Flag");
    ui->customPlot_m->yAxis->setRange(-1, 1);  // Ajuste conforme necessário

    ui->customPlot1_m->addGraph(); // Gráfico 1
    ui->customPlot1_m->addGraph(); // Gráfico 2
    ui->customPlot1_m->addGraph(); // Gráfico 3
    ui->customPlot1_m->addGraph(); // Gráfico 1
    ui->customPlot1_m->addGraph(); // Gráfico 2
    ui->customPlot1_m->addGraph(); // Gráfico 3
    // Configurar as propriedades dos gráficos
    ui->customPlot1_m->graph(0)->setPen(customPen1); // Configurar as propriedades do Gráfico 1
    ui->customPlot1_m->graph(1)->setPen(customPen2); // Configurar as propriedades do Gráfico 2
    ui->customPlot1_m->graph(2)->setPen(customPen3); // Configurar as propriedades do Gráfico 3
    ui->customPlot1_m->graph(3)->setPen(customPen4);
    ui->customPlot1_m->graph(4)->setPen(customPen5);
    ui->customPlot1_m->graph(5)->setPen(customPen6);
    ui->customPlot1_m->xAxis->setLabel("Tempo");
    ui->customPlot1_m->yAxis->setLabel("h1h");
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
    ui->customPlot2_m->yAxis->setLabel("h3h");
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
    QByteArray error = process->readAllStandardError();
    qDebug() << "Erro Python:" << error;
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

        connect(myMethod, &MyMethod::sendPyVector, this, &MainWindow::sendVectorPy);

        if(clientThread != nullptr)
        {
            connect(this, &MainWindow::amostragem, myMethod, &MyMethod::onFftTimer);
            plotTimer_m->start(200);
        }
        else ui->textEdit->append("Monitoramento do SV não esta habilitado!\n");

        myMethod->start();

        ui->iniMetButon->setText("Parar o Metodo");

    }else{
        disconnect(this, &MainWindow::amostragem, myMethod, &MyMethod::onFftTimer);
        plotTimer_m->stop();
        myMethod->requestInterruption();
    }
}
void MainWindow::on_spButon_clicked()
{
    if(myMethod != nullptr) myMethod->trigger_flag = true;
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
void MainWindow::on_actionSNR_triggered()
{
    bool ok;
    dBsnr = QInputDialog::getDouble(nullptr, "Digite um valor", "Digite o SNR:", dBsnr, -9999999.0, 9999999.0, 2, &ok);

    // Verificar se o usuário pressionou OK e inseriu um valor válido
    if (ok) {
        qDebug() << "Valor inserido:" << dBsnr;
    } else {
        qDebug() << "Operação cancelada pelo usuário.";
    }
}
/////////////////////////////////////////////////////////////////////////////
/// \brief MainWindow::onCalculate
///
float MainWindow::calculateMean(const QVector<float>& vec) {
    float sum = 0.0;
    for (const auto& value : vec) {
        sum += value;
    }
    return sum / vec.size();
}

// Função para calcular a potência de um QVector
float MainWindow::calculatePower(const QVector<float>& vec)
{
    float sumOfSquares = 0.0;
    for (const auto& value : vec) {
        sumOfSquares += value * value;
    }
    return sumOfSquares / vec.size();
}

// Função para adicionar ruído AWGN ao QVector
float MainWindow::calAWGN(QVector<float>& vec, float snr_dB)
{
    // Converter SNR de dB para uma relação linear
    float snr_linear = std::pow(10, snr_dB / 10.0);

    // Calcular a potência do sinal
    float signalPower = calculatePower(vec);

    // Calcular a potência do ruído
    float noisePower = signalPower / snr_linear;

    // Calcular o desvio padrão do ruído
    return std::sqrt(noisePower);
}
void MainWindow::on_actionget_desault_triggered()
{
    QVector<float> last100Elements = data[0].mid(data[0].size() - 160);
    stdDeviation[0] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[0];
    last100Elements = data[1].mid(data[1].size() - 160);
    stdDeviation[1] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[1];
    last100Elements = data[2].mid(data[2].size() - 160);
    stdDeviation[2] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[2];
    last100Elements = data[3].mid(data[3].size() - 160);
    stdDeviation[3] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[3];
    last100Elements = data[4].mid(data[4].size() - 160);
    stdDeviation[4] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[4];
    last100Elements = data[5].mid(data[5].size() - 160);
    stdDeviation[5] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[5];
    last100Elements = data[6].mid(data[6].size() - 160);
    stdDeviation[6] = calAWGN(last100Elements, dBsnr);
    qDebug() << stdDeviation[6];

}
/////////////////////////////////////////////////////////////////////////////
/// \brief MainWindow::onUpdateUI
///
void MainWindow::sendVectorPy(int tipo, float h1hM, float h1aM, float h3hM, float h3aM,
                              QVector<float> x1, QVector<float> x2, QVector<float> x3, quint64 dataIndex, int p1, int p3)
{
    creatPyConection();

    structPointerPy->tipo = tipo;
    structPointerPy->h1a_M = h1aM;
    structPointerPy->h1h_M = h1hM;
    structPointerPy->h3a_M = h3aM;
    structPointerPy->h3h_M = h3hM;
    structPointerPy->lenpos = p1;
    structPointerPy->lensig = p3;
    int len = 40*4096;

    QVector<float> xs;
    xs.resize(len);
    for(int i = 0; i > -len; i--){
        if(i >= -dataIndex)
            xs[len+i-1] = data[In].at(dataIndex+i);
        else
            xs[len+i-1] = 0;
    }
    std::copy(x1.begin(), x1.end(), structPointerPy->pyh1h);
    std::copy(x2.begin(), x2.end(), structPointerPy->pyh3h);
    std::copy(x3.begin(), x3.end(), structPointerPy->pyh3a);
    std::copy(xs.begin(), xs.end(), structPointerPy->signal);

    // Especifica o script Python a ser executado
    //QString scriptPath = "C:/Users/Aldair/GoogleDrive/Doutorado/PROJETOS/pyProjects/auxPyForDocPro/main.py";
    QString scriptPath = "C:/Users/Aldair/GoogleDrive/Doutorado/PROJETOS/pyProjects/auxPyForDoc_alg/main.py";

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
void MainWindow::onUpdateUI(const QString msg)
{
    ui->textEdit->append(msg);
}
void MainWindow::onUpdateData(const QVector<float> &vetor, const Timestamp &tempo)
{
    if(ui->actionRuido->isChecked())
    {
        double noise = (generator.generateDouble()*2 - 1)*stdDeviation[0];
        //float x = vetor[0] + noise;
        data[0].append(vetor[0] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[1];
        //x += vetor[1] + noise;
        data[1].append(vetor[1] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[2];
        //x += vetor[2] + noise;
        data[2].append(vetor[2] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[3];
        //data[3].append(x);
        data[3].append(vetor[3] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[4];
        data[4].append(vetor[4] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[5];
        data[5].append(vetor[5] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[6];
        data[6].append(vetor[6] + noise);

        noise = (generator.generateDouble()*2 - 1)*stdDeviation[7];
        data[7].append(vetor[7] + noise);
    }
    else
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

    quint64 t = Timestamp_getTimeInNs(const_cast<Timestamp*>(&tempo));
    dataTime.append(t);

    lastDataIndex = data[7].size() - 1;
    /// calcula a quantas amostras tem em um ciclo
    /// ref a tensao na fase A
    static float vaLast = 0.1;
    static qint64 zCrosLast = 0;
    static int count = 0;
    if(vetor[4] > 0 && vaLast <= 0)
    {
        SRate = data[4].size() - zCrosLast;
        freq = 1.0/((dataTime.last() - dataTime.at(zCrosLast - 1)) * 1e-9);
        zCrosLast = data[4].size();
        stepTime = (dataTime.last()-dataTime.at(dataTime.size() - 2)) * 1e-9;
        //qDebug() << stepTime;
    }
    vaLast = vetor[4];
    if(count >= 40){
        count = 0;
        emit amostragem();
    }else{
        count++;
    }

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
                ui->customPlot2->graph(1)->addData(i, data[0].at(i)+data[1].at(i)+data[2].at(i));
            }
            // Rescale e replot
            ui->customPlot->yAxis->setRange(minG1, maxG1);
            ui->customPlot->xAxis->setRange(len, 500, Qt::AlignRight);
            ui->customPlot1->yAxis->setRange(minG2, maxG2);
            ui->customPlot1->xAxis->setRange(len, 500, Qt::AlignRight);
            ui->customPlot2->yAxis->setRange(minG3-0.5, maxG3+0.5);
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
// Função para encontrar o menor e o maior valor dentro dos últimos 2000 elementos de um vetor de dados
void findMinMax(const QVector<float>& data, int startIndex, int endIndex, double& minValue, double& maxValue) {
    endIndex = qMin(endIndex, data.size() - 1);
    minValue = data[startIndex];
    maxValue = data[startIndex];

    for (int i = startIndex + 1; i <= endIndex; ++i) {
        minValue = qMin(minValue, data[i]);
        maxValue = qMax(maxValue, data[i]);
    }
}
void MainWindow::updatePlot_m()
{
    static int len = 0;
    static double maxG1 = 0;
    static double maxG2 = 0;
    static double minG1 = 0;
    static double minG2 = 0;
    int increment = 1;
    int size = 0;

    if(!myMethod->fftData[In][harm3].isEmpty())
    {
        size = myMethod->fftData[In][harm3].size();
        increment = size - len;
        if (increment > 0){

            for(int i=len; i<size; i++)
            {
                ui->customPlot_m->graph(0)->addData(i, myMethod->flags.f3h);
                ui->customPlot_m->graph(1)->addData(i, myMethod->flags.f1h*1.2);

                //ui->customPlot1_m->graph(0)->addData(i, myMethod->fftData[In][harm1].at(i));
                ui->customPlot1_m->graph(0)->addData(i, myMethod->modulo_predict[h1h].at(i));
                ui->customPlot1_m->graph(1)->addData(i, myMethod->bbv[h1h].at(i).BB_U);
                ui->customPlot1_m->graph(2)->addData(i, myMethod->bbv[h1h].at(i).BB_D);
                ui->customPlot1_m->graph(3)->addData(i, myMethod->bbv[h1h].at(i).SMA_longa);
                ui->customPlot1_m->graph(4)->addData(i, myMethod->bbv[h1h].at(i).SMA_curta);

                //ui->customPlot2_m->graph(0)->addData(i, myMethod->modulo_predict[h1h].at(i));
                ui->customPlot2_m->graph(0)->addData(i, myMethod->modulo_predict[h3h].at(i));
                ui->customPlot2_m->graph(1)->addData(i, myMethod->bbv[h3h].at(i).BB_U);
                ui->customPlot2_m->graph(2)->addData(i, myMethod->bbv[h3h].at(i).BB_D);
                ui->customPlot2_m->graph(3)->addData(i, myMethod->bbv[h3h].at(i).SMA_longa);
                ui->customPlot2_m->graph(4)->addData(i, myMethod->bbv[h3h].at(i).SMA_curta);
                //ui->customPlot2_m->graph(5)->addData(i, myMethod->flags.f3h);
                //ui->customPlot2_m->graph(5)->addData(len, fg);
            }
            mutex.lock();
            findMinMax(myMethod->modulo_predict[h1h], qMax(0, len-2000), len, minG1, maxG1);
            findMinMax(myMethod->modulo_predict[h3h], qMax(0, len-2000), len, minG2, maxG2);
            mutex.unlock();
            // Rescale e replot
            ui->customPlot_m->yAxis->setRange(-0.1, 1.3);
            ui->customPlot_m->xAxis->setRange(len, 2000, Qt::AlignRight);
            ui->customPlot1_m->yAxis->setRange(minG1-0.01, maxG1+0.01);
            ui->customPlot1_m->xAxis->setRange(len, 2000, Qt::AlignRight);
            ui->customPlot2_m->yAxis->setRange(minG2-0.01, maxG2+0.01);
            ui->customPlot2_m->xAxis->setRange(len, 2000, Qt::AlignRight);

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
void MainWindow::on_actionSet_svID_triggered()
{
    QDialog *janela1 = new QDialog(this);
    janela1->setObjectName("svID definition");
    janela1->setWindowTitle("svID definition");
    janela1->resize(366, 183);

    QVBoxLayout *layout = new QVBoxLayout(janela1);

    QLabel *label = new QLabel("Digite o svID:", janela1);
    layout->addWidget(label);

    QLineEdit *lineEdit = new QLineEdit(janela1);
    lineEdit->setText(svidref);
    layout->addWidget(lineEdit);

    // Adicionando entrada numérica float
    QLabel *floatLabel = new QLabel("Digite o Multiplicador do SV:", janela1);
    layout->addWidget(floatLabel);

    QDoubleSpinBox *doubleSpinBox = new QDoubleSpinBox(janela1);
    doubleSpinBox->setDecimals(2); // Define a quantidade de casas decimais
    doubleSpinBox->setMinimum(-1000000); // Defina o mínimo desejado, -1000 neste exemplo
    doubleSpinBox->setMaximum(1000000); // Defina o máximo desejado, 1000 neste exemplo
    doubleSpinBox->setValue(1.0/svMultiply);
    layout->addWidget(doubleSpinBox);

    // Adicionando caixa de seleção
    QLabel *comboBoxLabel = new QLabel("Selecione o tipo de entrada:", janela1);
    layout->addWidget(comboBoxLabel);

    QComboBox *comboBox = new QComboBox(janela1);
    comboBox->addItem("int");
    comboBox->addItem("float");
    comboBox->setCurrentIndex(svReadtype);
    layout->addWidget(comboBox);

    QPushButton *okButton = new QPushButton("OK", janela1);
    QPushButton *cancelButton = new QPushButton("Cancelar", janela1);
    layout->addWidget(okButton);
    layout->addWidget(cancelButton);

    // Conectar o sinal de clique do botão OK para aceitar o diálogo
    connect(okButton, &QPushButton::clicked, [this, lineEdit, doubleSpinBox, comboBox, janela1]() {
        svidref = lineEdit->text();
        svMultiply = 1.0/doubleSpinBox->value();
        svReadtype = comboBox->currentIndex();
        this->ui->textEdit->append(QString("svID setado para: %1").arg(svidref));
        this->ui->textEdit->append(QString("Número float digitado: %1").arg(doubleSpinBox->value()));
        this->ui->textEdit->append(QString("Tipo selecionado: %1").arg(comboBox->itemText(svReadtype)));
        janela1->accept();
    });

    connect(cancelButton, &QPushButton::clicked, [janela1]() {
        janela1->close();
    });

    janela1->show();

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
    static int lastcount = 0;
    static QStringList svIdList;
    int count = 0;

    ClientThread *instance = static_cast<ClientThread*>(parameter);
    QVector<float> vetor;
    vetor.resize(8);
    QString svID = QString::fromUtf8(SVSubscriber_ASDU_getSvId(asdu));

    if(!svIdList.contains(svID))
    {
        svIdList << svID;
        emit instance->updateUI(QString("svID=(%1)\n").arg(svID));

        emit instance->updateUI(QString("smpCnt: %1").arg(SVSubscriber_ASDU_getSmpCnt(asdu)));
        emit instance->updateUI(QString("confRev: %1").arg(SVSubscriber_ASDU_getConfRev(asdu)));
        //emit instance->updateUI(QString("Sample Rate: %1\n").arg(SVSubscriber_ASDU_getSmpRate(asdu)));
        //emit instance->updateUI(QString("Size in Bytes: %1\n").arg(SVSubscriber_ASDU_getDataSize(asdu)));
    }
    if (svidref.compare(svID) == 0){

        count = SVSubscriber_ASDU_getSmpCnt(asdu);
        if (count > lastcount + 1){
            instance->lossPacket = instance->lossPacket + count - (lastcount + 1);
        }
        lastcount = count;
        Timestamp tempo;

        if(svReadtype == 1){
            vetor[0] = SVSubscriber_ASDU_getFLOAT32(asdu, 0);
            vetor[1] = SVSubscriber_ASDU_getFLOAT32(asdu, 8);
            vetor[2] = SVSubscriber_ASDU_getFLOAT32(asdu, 16);
            vetor[3] = SVSubscriber_ASDU_getFLOAT32(asdu, 24);
            vetor[4] = SVSubscriber_ASDU_getFLOAT32(asdu, 32);
            vetor[5] = SVSubscriber_ASDU_getFLOAT32(asdu, 40);
            vetor[6] = SVSubscriber_ASDU_getFLOAT32(asdu, 48);
            vetor[7] = SVSubscriber_ASDU_getFLOAT32(asdu, 56);
        }else{
            vetor[0] = (float)SVSubscriber_ASDU_getINT32(asdu, 0) * svMultiply;
            vetor[1] = (float)SVSubscriber_ASDU_getINT32(asdu, 8) * svMultiply;
            vetor[2] = (float)SVSubscriber_ASDU_getINT32(asdu, 16) * svMultiply;
            vetor[3] = (float)SVSubscriber_ASDU_getINT32(asdu, 24) * svMultiply;
            vetor[4] = (float)SVSubscriber_ASDU_getINT32(asdu, 32) * svMultiply;
            vetor[5] = (float)SVSubscriber_ASDU_getINT32(asdu, 40) * svMultiply;
            vetor[6] = (float)SVSubscriber_ASDU_getINT32(asdu, 48) * svMultiply;
            vetor[7] = (float)SVSubscriber_ASDU_getINT32(asdu, 56) * svMultiply;
        }


        if (SVSubscriber_ASDU_getDataSize(asdu) > 64) {
            tempo = SVSubscriber_ASDU_getTimestamp(asdu, 64);
        }else{
            Timestamp_setTimeInNanoseconds(&tempo, Hal_getTimeInNs());
        }
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
    K.P_1h = 2048;
    K.P_3h = 2048;
    K.BB_w1h = 80;
    K.BB_w3h = 80;
    K.SMA_curta_w1h = 40;
    K.SMA_curta_w3h = 40;

    bbv[h1h].resize(181);
    bbv[h3h].resize(181);
    for (int i = 0; i < 181; ++i) {
        bbv[h1h][i] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        bbv[h3h][i] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    }

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

    harmList << 1 << 3;
    chList << In << K.phRef;

    flags.f1h=false;
    flags.f3h=false;
    flags.h1h_lenta = false;
    flags.h1h_rapida = false;
    flags.h3h_lenta = false;
    flags.h3h_rapida = false;
    flags.var_estavel_direcao_1h = false;
    flags.var_estavel_amplitude_1h = false;
    flags.var_abaixo_amplitude_1h = false;
    flags.var_estavel_direcao_3h = false;
    flags.var_estavel_amplitude_3h = false;
    flags.var_abaixo_amplitude_3h = false;

    count.h1h_rapida = 0;
    count.break_1h = 0;
    count.h3h_rapida = 0;
    count.t_h1h = 0;
    count.t_h3h = 0;

}
void MyMethod::sendDataPy(int x, quint64 ifft, quint64 dataIndex)
{
    QVector<float> x1;
    QVector<float> x2;
    QVector<float> x3;
    int p1 = 0;
    int p3 = 4095;
    int p = ifft - 4096;

    for (int i = p; i < ifft; i++)
    {
        if(i < 0){
            x1.append(0);
            x2.append(0);
            x3.append(0);
        }else{
            x1.append(modulo_predict[h1h].at(i));
            x2.append(modulo_predict[h3h].at(i));
            x3.append(Angulo_3h_ref1h.at(i));
        }
        if(i == count.h1h_position) p1 = x1.size();
        if(i == count.h3h_position) p3 = x1.size();
        if (x1.size() >= 4096) break;
    }

    emit sendPyVector(x, modulo_M[h1h].at(ifft), angulo_M[h1h].at(ifft),
                      modulo_M[h3h].at(ifft), angulo_M[h3h].at(ifft), x1 , x2, x3, dataIndex, p1, p3);

}
void MyMethod::onFftTimer()
{
    static quint64 ifft = 0;
    static qint64 ena_alg_count = 0;
    static bool ena_alg = false;
    quint64 dataIndex = *lastDataIndex;

    //-------- Calculo do FFT ----------

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
        //trigger_flag = true;
        angulo_M[h1h].append(angulo_M[h1h].last());
        modulo_M[h1h].append(modulo_M[h1h].last());
        angulo_M[h3h].append(angulo_M[h3h].last());
        modulo_M[h3h].append(modulo_M[h3h].last());
    }else{
        if(trigger_flag){
            angulo_M[h1h].append(phaseAngulo[In][harm1]);
            modulo_M[h1h].append(fftModulo[In][harm1]);
            angulo_M[h3h].append(phaseAngulo[In][harm3]);
            modulo_M[h3h].append(fftModulo[In][harm3]);
            trigger_flag = false;
        }else{
            QVector<float> result;
            result = mediadofasor(fftModulo[In][harm1],
                                                 phaseAngulo[In][harm1],
                                                 modulo_M[h1h].last(),
                                                 angulo_M[h1h].last(),
                                                 K.P_1h);
            angulo_M[h1h].append(result[0]);
            modulo_M[h1h].append(result[1]);

            result = mediadofasor(fftModulo[In][harm3],
                                                 phaseAngulo[In][harm3],
                                                 modulo_M[h3h].last(),
                                                 angulo_M[h3h].last(),
                                                 K.P_3h);
            angulo_M[h3h].append(result[0]);
            modulo_M[h3h].append(result[1]);
        }

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
    ena_alg_count++;
    if (ena_alg_count >= 240) ena_alg = true;
    else{
        flags.h1h_lenta = false;
        flags.h1h_rapida = false;
        count.h1h_rapida = 0;
        count.h1h_lenta = 0;
        ena_alg = false;
        count.h3h_rapida = 0;
        count.h3h_rapida = 0;
        flags.h3h_lenta = false;
        flags.h3h_rapida = false;
    }

    if (ifft < 180)
    {
        ifft++;
        return;
    }

    //-----------------------------------------------------
    static float BB_U_min_prev_1h = 0;
    static float BB_U_max_prev_1h = 0;
    static float BB_D_min_prev_1h = 0;
    static float BB_D_max_prev_1h = 0;
    static float SMA_longa_min_prev_1h = 0;
    static float SMA_longa_max_prev_1h = 0;
    static float BB_U_min_1h = 0;
    static float BB_U_max_1h = 0;
    static float BB_D_min_1h = 0;
    static float BB_D_max_1h = 0;
    static float SMA_longa_min_1h = 0;
    static float SMA_longa_max_1h = 0;

    static float BB_U_min_prev_3h = 0;
    static float BB_U_max_prev_3h = 0;
    static float BB_D_min_prev_3h = 0;
    static float BB_D_max_prev_3h = 0;
    static float SMA_longa_min_prev_3h = 0;
    static float SMA_longa_max_prev_3h = 0;
    static float BB_U_min = 0;
    static float BB_U_max = 0;
    static float BB_D_min = 0;
    static float BB_D_max = 0;
    static float SMA_longa_min = 0;
    static float SMA_longa_max = 0;
    //----------------Calcula medias 1h-------------------

    float SMA_longa_aux_1h = 0;
    for (int j = ifft;j >= ifft-K.BB_w1h+1; j--){
        SMA_longa_aux_1h = SMA_longa_aux_1h + modulo_predict[h1h].at(j);
    }
    bbp[h1h].SMA_longa = SMA_longa_aux_1h/K.BB_w1h;
    float SMA_curta_aux_1h = 0;
    for (int j = ifft; j >= ifft-K.SMA_curta_w1h+1; j--){
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

    if (bbp[h1h].BB_D > (bbp[h1h].BB_U-bbp[h1h].BB_D) &&
        bbp[h1h].SMA_curta > bbp[h1h].SMA_longa &&
        bbp[h1h].SMA_longa > bbv[h1h].at(ifft-60).SMA_longa &&
        bbp[h1h].SMA_curta >= 0.05)
    {
        if (count.h1h_lenta >= 10) flags.h1h_lenta = true;
        else count.h1h_lenta++;
    }
    else
    {
        count.h1h_lenta = 0;
        flags.h1h_lenta = false;
    }

    if (modulo_predict[h1h].last() > bbp[h1h].BB_U &&
        modulo_predict[h1h].last() >= 0.05)
    {
        if (count.h1h_rapida >= 2) flags.h1h_rapida = true;
        else count.h1h_rapida++;
    }
    else
    {
        count.h1h_rapida = 0;
        flags.h1h_rapida = false;
    }

    if ((flags.h1h_rapida == true || flags.h1h_lenta == true || flags.f3h) &&
        flags.f1h == false &&
        ena_alg)
    {
        flags.f1h = true;
        count.h1h_position = ifft;

        BB_U_min_prev_1h = bbp[h1h].BB_U;
        BB_U_max_prev_1h = bbp[h1h].BB_U;
        BB_D_min_prev_1h = bbp[h1h].BB_D;
        BB_D_max_prev_1h = bbp[h1h].BB_D;
        SMA_longa_min_prev_1h = bbp[h1h].SMA_longa;
        SMA_longa_max_prev_1h = bbp[h1h].SMA_longa;

        for (int j = ifft-60; j > ifft-180; j--){
            if (BB_U_min_prev_1h > bbv[h1h].at(j).BB_U)
                BB_U_min_prev_1h = bbv[h1h].at(j).BB_U;
            if (BB_U_max_prev_1h < bbv[h1h].at(j).BB_U)
                BB_U_max_prev_1h = bbv[h1h].at(j).BB_U;
            if (BB_D_min_prev_1h > bbv[h1h].at(j).BB_D)
                BB_D_min_prev_1h = bbv[h1h].at(j).BB_D;
            if (BB_D_max_prev_1h < bbv[h1h].at(j).BB_D)
                BB_D_max_prev_1h = bbv[h1h].at(j).BB_D;
            if (SMA_longa_min_prev_1h > bbv[h1h].at(j).SMA_longa)
                SMA_longa_min_prev_1h = bbv[h1h].at(j).SMA_longa;
            if (SMA_longa_max_prev_1h < bbv[h1h].at(j).SMA_longa)
                SMA_longa_max_prev_1h = bbv[h1h].at(j).SMA_longa;
        }
    }

    if (flags.f1h)
    {
        count.t_h1h++;

        BB_U_min_1h = bbp[h1h].BB_U;
        BB_U_max_1h = bbp[h1h].BB_U;
        BB_D_min_1h = bbp[h1h].BB_D;
        BB_D_max_1h = bbp[h1h].BB_D;
        SMA_longa_min_1h = bbp[h1h].SMA_longa;
        SMA_longa_max_1h = bbp[h1h].SMA_longa;

        for (int j = ifft; j > ifft-180; j--){
            if (BB_U_min_1h > bbv[h1h].at(j).BB_U)
                BB_U_min_1h = bbv[h1h].at(j).BB_U;
            if (BB_U_max_1h < bbv[h1h].at(j).BB_U)
                BB_U_max_1h = bbv[h1h].at(j).BB_U;
            if (BB_D_min_1h > bbv[h1h].at(j).BB_D)
                BB_D_min_1h = bbv[h1h].at(j).BB_D;
            if (BB_D_max_1h < bbv[h1h].at(j).BB_D)
                BB_D_max_1h = bbv[h1h].at(j).BB_D;
            if (SMA_longa_min_1h > bbv[h1h].at(j).SMA_longa)
                SMA_longa_min_1h = bbv[h1h].at(j).SMA_longa;
            if (SMA_longa_max_1h < bbv[h1h].at(j).SMA_longa)
                SMA_longa_max_1h = bbv[h1h].at(j).SMA_longa;
        }

        if (SMA_longa_max_1h < BB_U_min_1h && SMA_longa_min_1h > BB_D_max_1h)
             flags.var_estavel_direcao_1h = true;
        else flags.var_estavel_direcao_1h = false;

        if ((BB_U_max_1h-BB_D_min_1h) < 2*(BB_U_max_prev_1h-BB_D_min_prev_1h))
             flags.var_estavel_amplitude_1h = true;
        else flags.var_estavel_amplitude_1h = false;

        if ((BB_U_max_1h-BB_D_min_1h) < 0.04 )
             flags.var_abaixo_amplitude_1h = true;
        else flags.var_abaixo_amplitude_1h = false;

        if(((flags.var_estavel_direcao_1h && flags.var_estavel_amplitude_1h) || flags.var_abaixo_amplitude_1h) ||
            (count.t_h1h>=3072 && flags.f3h == false))
        {
            // qDebug() << SMA_longa_max_1h << BB_U_min_1h << SMA_longa_min_1h << BB_D_max_1h;
            // qDebug() << flags.var_estavel_direcao_1h;
            // qDebug() << BB_U_max_1h << BB_D_min_1h << BB_U_max_prev_1h << BB_D_min_prev_1h;
            // qDebug() << flags.var_estavel_amplitude_1h;
            // qDebug() << BB_U_max_1h << BB_D_min_1h;
            // qDebug() << flags.var_abaixo_amplitude_1h;

            count.break_1h++;
            if ((count.break_1h>=20 && bbp[h1h].SMA_curta<bbp[h1h].SMA_longa && count.t_h1h>=1024 && flags.f3h == false) ||
                (count.t_h1h>=3072 && flags.f3h == false))
            {
                flags.f1h = false;
                ena_alg_count = 0;
                count.break_1h = 0;
                count.t_h1h = 0;

                sendDataPy(1, ifft, dataIndex);
            }
        }
        else
        {
            count.break_1h = 0;
        }
    }
    else
    {
        count.t_h1h = 0;
    }



    //----------------Calcula medias-------------------

    float SMA_longa_aux_3h = 0;
    for (int j = ifft;j >= ifft-K.BB_w3h+1; j--){
        SMA_longa_aux_3h = SMA_longa_aux_3h + modulo_predict[h3h].at(j);
    }
    bbp[h3h].SMA_longa = SMA_longa_aux_3h/K.BB_w3h;
    float SMA_curta_aux_3h = 0;
    for (int j = ifft;j >= ifft-K.SMA_curta_w3h+1; j--){
        SMA_curta_aux_3h = SMA_curta_aux_3h + modulo_predict[h3h].at(j);
    }
    bbp[h3h].SMA_curta = SMA_curta_aux_3h/K.SMA_curta_w3h;
    float aux1 = 0;
    for (int j = ifft;j > ifft-K.BB_w3h; j--){
        aux1 = aux1 + qPow(modulo_predict[h3h].at(j) - bbp[h3h].SMA_longa, 2);
    }
    bbp[h3h].BB_DP = sqrt(aux1/(K.BB_w3h-1));
    bbp[h3h].BB_U = bbp[h3h].SMA_longa + 2*bbp[h3h].BB_DP;
    bbp[h3h].BB_D = bbp[h3h].SMA_longa - 2*bbp[h3h].BB_DP;
    bbv[h3h].append(bbp[h3h]);

    //------------------------------------------

    if (bbp[h3h].BB_D > ( bbp[h3h].BB_U-bbp[h3h].BB_D ) &&
        bbp[h3h].SMA_curta > bbp[h3h].SMA_longa &&
        bbp[h3h].SMA_longa > bbv[h3h].at(ifft-60).SMA_longa &&
        bbp[h3h].SMA_curta >= 0.025 )
    {
        if (count.h3h_lenta >= 10) flags.h3h_lenta = true;
        else count.h3h_lenta++;
    }
    else
    {
        count.h3h_lenta = 0;
        flags.h3h_lenta = false;
    }

    if (modulo_predict[h3h].last() > bbp[h3h].BB_U &&
        bbp[h3h].SMA_curta >= 0.025)
    {
        //qDebug() << modulo_predict[h3h].last() << bbp[h3h].BB_U;
        if (count.h3h_rapida >= 3) flags.h3h_rapida = true;
        else count.h3h_rapida++;
    }
    else
    {
        count.h3h_rapida = 0;
        flags.h3h_rapida = false;
    }

    if ((flags.h3h_lenta || flags.h3h_rapida) &&
        flags.f3h == false &&
        ifft > 600 &&
        ena_alg)
    {
        // qDebug() << flags.h3h_lenta << flags.h3h_rapida;

        flags.f3h = true;

        flags.h3h_lenta = false;
        flags.h3h_rapida = false;

        count.h3h_position = ifft;

        BB_U_min_prev_3h = bbp[h3h].BB_U;
        BB_U_max_prev_3h = bbp[h3h].BB_U;
        BB_D_min_prev_3h = bbp[h3h].BB_D;
        BB_D_max_prev_3h = bbp[h3h].BB_D;
        SMA_longa_min_prev_3h = bbp[h3h].SMA_longa;
        SMA_longa_max_prev_3h = bbp[h3h].SMA_longa;

        for (int j = ifft-60; j > ifft-180; j--){
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

    if (flags.f3h)
    {
        count.t_h3h++;

        BB_U_min = bbp[h3h].BB_U;
        BB_U_max = bbp[h3h].BB_U;
        BB_D_min = bbp[h3h].BB_D;
        BB_D_max = bbp[h3h].BB_D;
        SMA_longa_min = bbp[h3h].SMA_longa;
        SMA_longa_max = bbp[h3h].SMA_longa;

        for (int j = ifft; j > ifft-180; j--){
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

        if (SMA_longa_max < BB_U_min && SMA_longa_min > BB_D_max)
             flags.var_estavel_direcao_3h = true;
        else flags.var_estavel_direcao_3h = false;

        if ((BB_U_max-BB_D_min) < 2*(BB_U_max_prev_3h-BB_D_min_prev_3h))
             flags.var_estavel_amplitude_3h = true;
        else flags.var_estavel_amplitude_3h = false;

        if ((BB_U_max-BB_D_min) < 0.04)
             flags.var_abaixo_amplitude_3h = true;
        else flags.var_abaixo_amplitude_3h = false;

        if(((((flags.var_estavel_direcao_3h && flags.var_estavel_amplitude_3h) || flags.var_abaixo_amplitude_3h) &&
              count.break_1h >= 20) || count.t_h1h>=3072 || count.t_h3h>=3072) && count.t_h3h>=1024)
        {
            ena_alg_count = 0;
            flags.f3h = false;
            flags.f1h = false;
            flags.var_estavel_direcao_3h = false;
            flags.var_estavel_amplitude_3h = false;
            flags.var_abaixo_amplitude_3h = false;

            sendDataPy(2, ifft, dataIndex);
        }
    }
    else
    {
        count.t_h3h = 0;
    }

    ifft++;
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














