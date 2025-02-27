#include "WebSocketServer.h"
#include <QDebug>

WebSocketServer::WebSocketServer(QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(nullptr)
{
    serialPort = new QSerialPort(this);
    openSerialPort();
    connect(serialPort, &QSerialPort::readyRead, this, &WebSocketServer::readSerialData);
}

WebSocketServer::~WebSocketServer()
{
    // 断开所有客户端连接
    for (QWebSocket* client : m_clients)
    {
        client->close();
        client->deleteLater();
    }
    delete m_pWebSocketServer;
}

bool WebSocketServer::startServer(quint16 port)
{
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("WebSocket Server"),
        QWebSocketServer::NonSecureMode, this);
    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
    {
        qDebug() << "WebSocket server listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
        return true;
    }
    else
    {
        qDebug() << "Failed to start WebSocket server";
        return false;
    }
}

void WebSocketServer::openSerialPort()
{
    // 获取系统中的所有串口信息
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    if (availablePorts.isEmpty())
    {
        qDebug() << "没有可用串口";
        return;
    }
    // 选择第一个可用串口
    QString portName = availablePorts.first().portName();
    qDebug() << "选择的串口: " << portName;
    // 设置串口信息
    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    // 打开串口
    if (serialPort->open(QIODevice::ReadWrite))
    {
        qDebug() << "成功打开串口" << portName;
        // 向串口发送初始化命令
        serialPort->write("AT+CGPS=1\r\n");
        if (serialPort->waitForBytesWritten(1000))
        {
            qDebug() << "sned ok";
        }
        else
        {
            qDebug() << "send failed";
        }
    }
    else
    {
        qDebug() << "打开串口失败: " << serialPort->errorString();
    }
}

void WebSocketServer::readSerialData()
{
    // 读取串口数据
    QByteArray data = serialPort->readAll();
    qDebug() << data;
    //-----------------------------------------------------------------//
    //提取经纬度、高度
    GPSData_string gpsData_single = parseGPSData(QString(data));
    // 输出保存的精确数据
    qDebug() << "Latitude:" << gpsData_single.latitude;
    qDebug() << "Longitude:" << gpsData_single.longitude;
    qDebug() << "Altitude:" << gpsData_single.altitude;
    // 创建一个新的 GPSData 结构体
    GPSData gpsData_new;
    //直接将纬度转换为 double 类型并保留小数精度
    gpsData_new.latitude = gpsData_single.latitude.toDouble();  // 转换为 double
    gpsData_new.longitude = gpsData_single.longitude.toDouble();  // 转换为 double
    gpsData_new.altitude = gpsData_single.altitude;
    gpsString = gpsDataToString(gpsData_new);
    sendFormattedData(gpsString);
}


void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    m_clients.append(pSocket);  // 添加到客户端列表
    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessageReceived);
    connect(pSocket, &QWebSocket::disconnected, this, [this, pSocket]()
    {
        m_clients.removeAll(pSocket);  // 移除断开连接的客户端
        pSocket->deleteLater();
    });
    qDebug() << "New connection established";
}

void WebSocketServer::onTextMessageReceived(const QString &message)
{
    qDebug() << "Message received:" << message;
    // Echo the message back to the client
    for (QWebSocket *pSocket : qAsConst(m_clients))
    {
        pSocket->sendTextMessage("Echo: " + message);
    }
}

GPSData_string WebSocketServer::parseGPSData(const QString &input)
{
    GPSData_string data = {0, 0, 0};
    // 正则表达式匹配经纬度和高度
    QRegularExpression regex(R"(\+CGPS:([\-0-9\.]+),([NS]),([\-0-9\.]+),([EW]),(\d+))");
    QRegularExpressionMatch match = regex.match(input);
    if (match.hasMatch())
    {
        // 获取经纬度的字符串值，保持原始精度
        QString lat = match.captured(1);
        QString latDirection = match.captured(2);
        QString lon = match.captured(3);
        QString lonDirection = match.captured(4);
        double altitude = match.captured(5).toDouble();
        // 根据方向调整经纬度值
        if (latDirection == "S")
        {
            lat = "-" + lat;
        }
        if (lonDirection == "W")
        {
            lon = "-" + lon;
        }
        // 设置返回的结构体
        data.latitude = lat;
        data.longitude = lon;
        data.altitude = altitude;
    }
    else
    {
        qWarning() << "Invalid GPS data format!";
    }
    return data;
}

QString WebSocketServer::gpsDataToString(const GPSData &data)
{
    return QString("%1,%2,%3")
           .arg(data.latitude, 0, 'f', 6)  // 纬度，显示6位小数
           .arg(data.longitude, 0, 'f', 6) // 经度，显示6位小数
           .arg(data.altitude, 0, 'f', 2); // 高度，显示2位小数
}

void WebSocketServer::sendFormattedData(const QString &formattedOutput)
{
    for (QWebSocket *pSocket : qAsConst(m_clients))
    {
        pSocket->sendTextMessage(formattedOutput);
        qDebug() << "Sent data: " << formattedOutput;
    }
}
