#include <QCoreApplication>
#include <QDebug>
#include <QUrl>

#include "httpclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    HttpClient client;

    HttpRequest request;
    request.method = HttpMethod::Get;
    request.url = QUrl(QStringLiteral("https://httpbin.org/get"));
    request.timeoutMs = 5000;

    const HttpResponse response = client.fetch(request, RequestMode::Sync);
    if (response.success) {
        qDebug() << "Status:" << response.statusCode;
        qDebug().noquote() << response.payload;
    } else {
        qWarning() << response.errorString;
    }

    return 0;
}
