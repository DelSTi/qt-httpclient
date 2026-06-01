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
class QNetworkRequest;

enum class HttpClientMethod {
    Get,
    Post,
    Put,
    Delete,
    Head,
    Patch,
    Options
};

enum class HttpClientRequestMode {
    Async,
    Sync
};

struct HttpClientSslOptions
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

struct HttpClientRequest
{
    HttpClientMethod method = HttpClientMethod::Get;
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray payload;
    int timeoutMs = 0;
    HttpClientSslOptions sslOptions;
};

struct HttpClientResponse
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

    HttpClientResponse fetch(const HttpClientRequest &httpRequest, HttpClientRequestMode mode = HttpClientRequestMode::Async);
    HttpClientResponse waitForFinish(QNetworkReply *reply);

signals:
    void finished(const HttpClientResponse &response);

private:
    QNetworkAccessManager *m_networkManager;
    void handleFinished(QNetworkReply *reply);
    HttpClientResponse createResponse(QNetworkReply *reply) const;
    void applySslOptions(QNetworkRequest &request, const HttpClientRequest &httpRequest) const;
    void handleSslErrors(QNetworkReply *reply, const HttpClientRequest &httpRequest) const;
};

#endif // HTTPCLIENT_H
