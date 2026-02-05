# Qt HttpClient

Lightweight wrapper around `QNetworkAccessManager` that provides a simple API for sending HTTP requests in Qt projects.

## Features
- Standard HTTP methods support (GET, POST, PUT, DELETE, HEAD) and custom ones (PATCH, OPTIONS).
- Asynchronous and synchronous modes (`RequestMode::Async`/`RequestMode::Sync`).
- Ready-to-use `HttpRequest` and `HttpResponse` structures for request/response data.
- `finished(const HttpResponse &)` signal for async mode callbacks.
- Per-request `timeoutMs`.
- Optional SSL settings: `QSslConfiguration`, certificate chains, private keys, and verification control.

## Structure
```
qt-httpclient/
  httpclient/
    httpclient.h
    httpclient.cpp
    httpclient.pri
  examples/
    simple_demo/
      simple_demo.pro
      main.cpp
  README.md
```

## Integrating into a project
1. In your `*.pro` file, add:
   ```prolog
   QT += network
   include(httpclient/httpclient.pri)
   ```
2. Build the project — Qt Creator will include the client sources automatically.

## Usage example
```cpp
HttpClient client;

HttpRequest request;
request.method = HttpMethod::Post;
request.url = QUrl(QStringLiteral("https://example.com/api"));
request.headers.append({"Content-Type", "application/json"});
request.payload = R"({"key":"value"})";
request.timeoutMs = 5000; // 5 секунд
request.sslOptions.enabled = true;
request.sslOptions.ignoreSslErrors = false;

QObject::connect(&client, &HttpClient::finished, [](const HttpResponse &response) {
    if (response.success) {
        qDebug() << "Status:" << response.statusCode << "Payload:" << response.payload;
    } else {
        qWarning() << response.errorString;
    }
});

client.fetch(request, RequestMode::Async);       // асинхронный вызов
const auto syncResponse = client.fetch(request, RequestMode::Sync); // синхронный вызов
```

## Error handling
- Invalid URL returns an `HttpResponse` with status `-1` and a filled `errorString`.
- When `timeoutMs` elapses, the request is aborted and `errorString` contains `Request timeout after <ms> ms`.
- On network errors, `errorString` contains the `QNetworkReply` error code and text like `"<code> - <message>"`.

## Example project
The example is located in `examples/simple_demo`. Build it with:
```bash
qmake && make
```

## SSL settings
- Enable SSL with `request.sslOptions.enabled = true;`.
- For advanced setup, pass a custom `QSslConfiguration` or set specific parameters:
  ```cpp
  request.sslOptions.localCertificates = {clientCert};
  request.sslOptions.localCertificate = clientCert; // explicit certificate selection
  request.sslOptions.privateKey = clientKey;
  request.sslOptions.caCertificates = QSslCertificate::fromPath(":/certs/ca.pem");
  request.sslOptions.peerVerifyMode = QSslSocket::VerifyPeer;
  request.sslOptions.peerVerifyName = QStringLiteral("example.com");
  ```
- For debugging, you may disable checks with `request.sslOptions.ignoreSslErrors = true;`, but this is not recommended for production.

### `QSslConfiguration` example
```cpp
HttpRequest request;
request.url = url;
request.sslOptions.enabled = true;
request.sslOptions.useCustomConfiguration = true;

QSslConfiguration sslConfiguration;
QFile certificateFile(":/certs/client.crt");
if (certificateFile.exists() && certificateFile.open(QIODevice::ReadOnly)) {
    QSslCertificate certificate(&certificateFile, QSsl::Pem);
    sslConfiguration.setLocalCertificate(certificate);
}

QFile privateKeyFile(":/certs/client.key");
if (privateKeyFile.exists() && privateKeyFile.open(QIODevice::ReadOnly)) {
    QSslKey privateKey(&privateKeyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    sslConfiguration.setPrivateKey(privateKey);
}

request.sslOptions.configuration = sslConfiguration;

HttpClient client;
client.fetch(request);
```

## Notes
- In `RequestMode::Sync` without `timeoutMs`, the wait can be infinite (if the server never responds).

## Extending
- Add custom headers using `headers` as a list of `QPair<QByteArray, QByteArray>`.
- For other HTTP methods, use `sendCustomRequest` similar to the PATCH implementation.
