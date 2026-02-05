#include "httpclient.h"

#include <QBuffer>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariant>
#include <QSslError>

namespace {
constexpr char kReplyHandledProperty[] = "httpClientHandled";
constexpr char kReplyTimeoutProperty[] = "httpClientTimeout";
}

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &HttpClient::handleFinished);
}

HttpResponse HttpClient::fetch(const HttpRequest &httpRequest, RequestMode mode)
{
    const bool asyncMode = (mode == RequestMode::Async);
    auto makeErrorResponse = [](const QString &error) {
        return HttpResponse{false, -1, {}, error};
    };
    auto handleImmediateError = [this, asyncMode](const HttpResponse &response) {
        if (asyncMode) {
            emit finished(response);
        }
        return response;
    };

    if (!httpRequest.url.isValid() || httpRequest.url.isEmpty()) {
        const QString detail = tr("Invalid URL: %1").arg(httpRequest.url.toString());
        return handleImmediateError(makeErrorResponse(detail));
    }

    QNetworkRequest request(httpRequest.url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    for (const auto &header : httpRequest.headers) {
        if (!header.first.isEmpty()) {
            request.setRawHeader(header.first, header.second);
        }
    }
    applySslOptions(request, httpRequest);

    QNetworkReply *reply = nullptr;
    const HttpMethod method = httpRequest.method;
    const QByteArray payload = httpRequest.payload;
    const int timeoutMs = httpRequest.timeoutMs > 0 ? httpRequest.timeoutMs : 0;

    switch (method) {
    case HttpMethod::Get:
        reply = m_networkManager->get(request);
        break;
    case HttpMethod::Post:
        reply = m_networkManager->post(request, payload);
        break;
    case HttpMethod::Put:
        reply = m_networkManager->put(request, payload);
        break;
    case HttpMethod::Delete:
        reply = m_networkManager->deleteResource(request);
        break;
    case HttpMethod::Head:
        reply = m_networkManager->head(request);
        break;
    case HttpMethod::Patch: {
        auto *buffer = new QBuffer;
        buffer->setData(payload);
        buffer->open(QIODevice::ReadOnly);
        reply = m_networkManager->sendCustomRequest(request, QByteArrayLiteral("PATCH"), buffer);
        if (reply) {
            buffer->setParent(reply);
        } else {
            buffer->deleteLater();
        }
        break;
    }
    case HttpMethod::Options:
    {
        auto *buffer = new QBuffer;
        buffer->setData(payload);
        buffer->open(QIODevice::ReadOnly);
        reply = m_networkManager->sendCustomRequest(request, QByteArrayLiteral("OPTIONS"), buffer);
        if (reply) {
            buffer->setParent(reply);
        } else {
            buffer->deleteLater();
        }
        break;
    }
    }

    if (!reply) {
        return handleImmediateError(makeErrorResponse(tr("Unsupported HTTP method")));
    }

    handleSslErrors(reply, httpRequest);

    if (timeoutMs > 0) {
        auto *timer = new QTimer(reply);
        timer->setSingleShot(true);
        timer->setInterval(timeoutMs);
        connect(timer, &QTimer::timeout, reply, [reply, timeoutMs]() {
            if (reply->isFinished()) {
                return;
            }
            reply->setProperty(kReplyTimeoutProperty, timeoutMs);
            reply->abort();
        });
        connect(reply, &QNetworkReply::finished, timer, &QTimer::stop);
        timer->start();
    }

    if (!asyncMode) {
        connect(reply, &QNetworkReply::finished, reply, [reply]() {
            reply->setProperty(kReplyHandledProperty, true);
        });
    }

    if (!asyncMode) {
        return waitForFinish(reply);
    }

    return {};
}

HttpResponse HttpClient::waitForFinish(QNetworkReply *reply)
{
    if (!reply) {
        return {};
    }

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->setProperty(kReplyHandledProperty, true);
    const HttpResponse response = createResponse(reply);
    reply->deleteLater();
    return response;
}

void HttpClient::handleFinished(QNetworkReply *reply)
{
    if (!reply || reply->property(kReplyHandledProperty).toBool()) {
        return;
    }

    reply->setProperty(kReplyHandledProperty, true);
    const HttpResponse response = createResponse(reply);
    emit finished(response);
    reply->deleteLater();
}

HttpResponse HttpClient::createResponse(QNetworkReply *reply) const
{
    if (!reply) {
        return {};
    }

    const QVariant statusAttribute = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int statusCode = statusAttribute.isValid() ? statusAttribute.toInt() : -1;
    HttpResponse response{
        reply->error() == QNetworkReply::NoError,
        statusCode,
        reply->readAll(),
        {}
    };
    const int timeoutMs = reply->property(kReplyTimeoutProperty).toInt();
    if (timeoutMs > 0 && !response.success) {
        response.success = false;
        response.errorString = tr("Request timeout after %1 ms").arg(timeoutMs);
    } else if (!response.success) {
        const int networkErrorCode = static_cast<int>(reply->error());
        response.errorString = QStringLiteral("%1 - %2").arg(networkErrorCode).arg(reply->errorString());
    }

    return response;
}

void HttpClient::applySslOptions(QNetworkRequest &request, const HttpRequest &httpRequest) const
{
    const HttpSslOptions &ssl = httpRequest.sslOptions;
    if (!ssl.enabled) {
        return;
    }

    QSslConfiguration configuration = ssl.useCustomConfiguration
            ? ssl.configuration
            : QSslConfiguration::defaultConfiguration();

    if (!ssl.caCertificates.isEmpty()) {
        configuration.setCaCertificates(ssl.caCertificates);
    }

    if (!ssl.localCertificates.isEmpty()) {
        configuration.setLocalCertificateChain(ssl.localCertificates);
        if (ssl.localCertificate.isNull()) {
            configuration.setLocalCertificate(ssl.localCertificates.first());
        }
    }

    if (!ssl.localCertificate.isNull()) {
        configuration.setLocalCertificate(ssl.localCertificate);
    }

    if (!ssl.privateKey.isNull()) {
        configuration.setPrivateKey(ssl.privateKey);
    }

    if (!ssl.peerVerifyName.isEmpty()) {
        configuration.setPeerVerifyName(ssl.peerVerifyName);
    }

    configuration.setPeerVerifyMode(ssl.peerVerifyMode);
    request.setSslConfiguration(configuration);
}

void HttpClient::handleSslErrors(QNetworkReply *reply, const HttpRequest &httpRequest) const
{
    if (!reply) {
        return;
    }

    const HttpSslOptions &ssl = httpRequest.sslOptions;
    if (!ssl.enabled || !ssl.ignoreSslErrors) {
        return;
    }

    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &errors) {
        reply->ignoreSslErrors(errors);
    });
}
