#include <QCoreApplication>
#include "WebSocketServer.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
    WebSocketServer webSocketServer;
    if (!webSocketServer.startServer(8008))
    {
        qCritical() << "Failed to start WebSocket server.";
        return -1;
    }
    // Send some test data after the server starts
    QTimer::singleShot(5000, [&]()
    {
        webSocketServer.sendFormattedData("Hello WebSocket Clients!");
    });
    return a.exec();
}
