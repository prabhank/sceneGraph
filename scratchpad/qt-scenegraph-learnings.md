Use some tips for metrics from below code
    https://github.com/KDAB/GammaRay



Text.

Manual glyph approach: If you absolutely must remain in C++ with no private classes, you can do a partial approach by using a QRawFont to get glyph data, then uploading it to geometry. But that basically reimplements a chunk of QQuickTextNode.
