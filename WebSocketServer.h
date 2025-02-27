#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include<QWebSocket>
#include<QWebSocketServer>
#include<QList>
#include<QString>
#include<QTimer>
#include<QFileInfoList>
#include<QDir>
#include<QLabel>
#include<QString>
#include<QLineEdit>
#include<QHBoxLayout>
#include<QSerialPort>
#include<QSerialPortInfo>
#include<QComboBox>


struct GPSData
{
    double latitude;  // 使用QString保存原始字符串
    double longitude; // 使用QString保存原始字符串
    double altitude;
};

struct GPSData_string
{
    QString latitude;  // 使用QString保存原始字符串
    QString longitude; // 使用QString保存原始字符串
    double altitude;
};

class WebSocketServer : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();

    bool startServer(quint16 port);
    // 串口相关槽函数
    void openSerialPort();
    void readSerialData();  // 读取串口数据
    // 将 sendFormattedData 方法设为公共
    void sendFormattedData(const QString &formattedOutput);  // 公共方法

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    //正则化，提取三个数据
    GPSData_string parseGPSData(const QString &input);
    QString gpsDataToString(const GPSData &data);

private:
    QString gpsString;
    QComboBox* serialPortComboBox;  // 串口选择下拉框
    QSerialPort* serialPort;  // 串口对象
    QWebSocketServer* m_pWebSocketServer;
    QList<QWebSocket*> m_clients;  // 用于保存所有客户端的列表
};

#endif // WEBSOCKETSERVER_H
