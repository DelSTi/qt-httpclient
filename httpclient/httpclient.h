#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QUrl>

#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>

class QNetworkAccessManager;
class QNetworkReply;

enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete,
    Head,
    Patch,
    Options
};

enum class RequestMode {
    Async,
    Sync
};

struct HttpSslOptions
{
    bool enabled = false;
    bool useCustomConfiguration = false;
    bool ignoreSslErrors = false;
    QSslConfiguration configuration;
    QList<QSslCertificate> caCertificates;
    QList<QSslCertificate> localCertificates;
    QSslCertificate localCertificate;
    QSslKey privateKey;
    QString peerVerifyName;
    QSslSocket::PeerVerifyMode peerVerifyMode = QSslSocket::AutoVerifyPeer;
};

struct HttpRequest
{
    HttpMethod method = HttpMethod::Get;
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray payload;
    int timeoutMs = 0;
    HttpSslOptions sslOptions;
};

struct HttpResponse
{
    bool success = false;
    int statusCode = -1;
    QByteArray payload;
    QString errorString;
};

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);

    HttpResponse fetch(const HttpRequest &httpRequest, RequestMode mode = RequestMode::Async);
    HttpResponse waitForFinish(QNetworkReply *reply);

signals:
    void finished(const HttpResponse &response);

private:
    QNetworkAccessManager *m_networkManager;
    void handleFinished(QNetworkReply *reply);
    HttpResponse createResponse(QNetworkReply *reply) const;
    void applySslOptions(QNetworkRequest &request, const HttpRequest &httpRequest) const;
    void handleSslErrors(QNetworkReply *reply, const HttpRequest &httpRequest) const;
};

#endif // HTTPCLIENT_H
