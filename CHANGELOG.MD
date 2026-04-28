# Changelog

## 2026-04-28
- Removed local `asyncMode` variable in `fetch()` and switched to direct `RequestMode` comparisons at usage points.

## 2026-02-05
- Added payload support for `OPTIONS` requests.
- Prevented timeout handling from overriding successful responses by stopping the timeout timer on `finished`.
- Avoided marking replies as handled too early in sync mode.
- Added explicit `localCertificate` support in SSL options.
- Documented that sync mode without `timeoutMs` can wait indefinitely.

## 2026-01-15
- Initial implementation of the Qt `HttpClient` wrapper.
- Support for standard HTTP methods and custom methods (PATCH, OPTIONS).
- Async and sync request modes.
- Request/response data structures with headers, payload, and timeout.
- SSL configuration and certificate handling.
