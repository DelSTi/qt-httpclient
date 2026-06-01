#include <QCoreApplication>
#include <QDebug>
#include <QUrl>

#include "httpclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    HttpClient client;

    HttpClientRequest request;
    request.method = HttpClientMethod::Get;
    request.url = QUrl(QStringLiteral("https://httpbin.org/get"));
    request.timeoutMs = 5000;

    const HttpClientResponse response = client.fetch(request, HttpClientRequestMode::Sync);
    if (response.success) {
        qDebug() << "Status:" << response.statusCode;
        qDebug().noquote() << response.payload;
    } else {
        qWarning() << response.errorString;
    }

    return 0;
}
