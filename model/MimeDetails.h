#ifndef MIMEDETAILS_H
#define MIMEDETAILS_H

#include <map>
#include <QString>

struct MimeDetails
{
    unsigned long long totalSize = 0;
    unsigned long fileCount = 0;
};

// MIME is kinb of misused and actually stands for file extension
struct TMimeDetailsList : std::map<
    QString,    // Mime type
    MimeDetails>
{
    inline static const QString ALL_MIMETYPE = "*";

    TMimeDetailsList();

    void addMimeDetails(
        QString mimeType,
        unsigned long long totalSize,
        unsigned long fileCount);

    void addMimeDetails(const TMimeDetailsList& mimeDetailsList);
};

#endif // MIMEDETAILS_H
