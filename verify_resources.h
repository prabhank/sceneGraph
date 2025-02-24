#ifndef VERIFY_RESOURCES_H
#define VERIFY_RESOURCES_H

#include <QString>

class ResourceVerifier {
public:
    static bool verifyImages();
    static bool verifyImage(const QString &path);
    static bool verifyResources();
private:
    static void debugImageInfo(const QString &path);
};

#endif // VERIFY_RESOURCES_H
