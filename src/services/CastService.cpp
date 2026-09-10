/*
 * CineWindows - Video Player
 * Copyright (c) 2026 Ritesh Pandit
 *
 * CineWindows Community License
 *
 * This source code is made available for personal, non-commercial
 * use only. Organizations may not use, copy, modify, or distribute
 * this code without written permission from Ritesh Pandit.
 *
 * See the LICENSE.md file for full license terms.
 *
 * Project: CineWindows
 * Author:  Ritesh Pandit
 * Last modified: 2026-09-10
 * Modified by: Ritesh Pandit
 */

#include "services/CastService.h"

#include <QNetworkDatagram>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QXmlStreamReader>

#include <utility>

namespace
{
constexpr int MaximumDeviceCandidates = 32;
constexpr qint64 MaximumDeviceResponseBytes = 1024 * 1024;
constexpr int DeviceRequestTimeoutMs = 5000;

bool isDeviceUrl(const QUrl& url, const QHostAddress& deviceAddress)
{
    QHostAddress urlAddress;
    const QString scheme = url.scheme().toLower();
    return url.isValid() && (scheme == QStringLiteral("http") || scheme == QStringLiteral("https"))
        && urlAddress.setAddress(url.host()) && urlAddress == deviceAddress;
}

void boundDeviceReply(QNetworkReply* reply)
{
    QObject::connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64 total) {
        if (received > MaximumDeviceResponseBytes || total > MaximumDeviceResponseBytes)
            reply->abort();
    });
}
}

/**
 * @brief Constructs a CastService and connects the UDP socket readyRead signal.
 * @param parent Optional QObject parent for Qt ownership.
 */
CastService::CastService(QObject* parent)
    : QObject(parent)
{
    // Process incoming SSDP responses as they arrive
    connect(&m_socket, &QUdpSocket::readyRead, this, &CastService::readDiscoveryResponses);
}

QVariantList CastService::devices() const { return m_devices; }
bool CastService::discovering() const { return m_discovering; }
QString CastService::statusMessage() const { return m_statusMessage; }

/**
 * @brief Sends an SSDP M-SEARCH multicast to discover DLNA MediaRenderer devices.
 */
void CastService::discover()
{
    const int generation = ++m_discoveryGeneration;
    // Clear any previously discovered devices
    m_devices.clear();
    m_locations.clear();
    Q_EMIT devicesChanged();
    m_socket.close();
    // Bind a UDP socket to receive SSDP discovery responses
    if (!m_socket.bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress))
    {
        setStatusMessage(tr("Could not start device discovery"));
        return;
    }
    // Send SSDP M-SEARCH for MediaRenderer devices
    const QByteArray request = QByteArrayLiteral(
        "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\n"
        "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n\r\n");
    m_socket.writeDatagram(request, QHostAddress(QStringLiteral("239.255.255.250")), 1900);
    setDiscovering(true);
    setStatusMessage(tr("Searching for DLNA media renderers..."));
    // Auto-stop discovery after 3.5 seconds
    QTimer::singleShot(3500, this, [this, generation] {
        if (generation != m_discoveryGeneration)
            return;
        m_socket.close();
        setDiscovering(false);
        setStatusMessage(m_devices.isEmpty() ? tr("No compatible devices found")
                                             : tr("Found %1 devices").arg(m_devices.size()));
    });
}

/**
 * @brief Reads and parses incoming SSDP UDP datagrams for device locations.
 */
void CastService::readDiscoveryResponses()
{
    // Drain all pending UDP datagrams
    while (m_socket.hasPendingDatagrams())
    {
        const QNetworkDatagram datagram = m_socket.receiveDatagram();
        if (!m_discovering)
            continue;
        const QByteArray payload = datagram.data();
        // Extract the LOCATION header from each SSDP response
        for (const QByteArray& rawLine : payload.split('\n'))
        {
            const QByteArray line = rawLine.trimmed();
            if (!line.toLower().startsWith("location:"))
                continue;
            const QUrl location(QString::fromUtf8(line.mid(line.indexOf(':') + 1).trimmed()));
            // Avoid re-inspecting the same device
            if (m_locations.size() < MaximumDeviceCandidates
                && isDeviceUrl(location, datagram.senderAddress())
                && !m_locations.contains(location.toString()))
            {
                m_locations.insert(location.toString());
                inspectDevice(location, datagram.senderAddress(), m_discoveryGeneration);
            }
        }
    }
}

/**
 * @brief Fetches a device's UPnP description XML and extracts the AVTransport control URL.
 * @param descriptionUrl URL to the device description document.
 */
void CastService::inspectDevice(const QUrl& descriptionUrl, const QHostAddress& deviceAddress, int generation)
{
    QNetworkRequest request(descriptionUrl);
    request.setTransferTimeout(DeviceRequestTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    QNetworkReply* reply = m_network.get(request);
    boundDeviceReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, deviceAddress, generation] {
        const QByteArray data = reply->readAll();
        const QUrl finalDescriptionUrl = reply->url();
        const bool ok = reply->error() == QNetworkReply::NoError
            && data.size() <= MaximumDeviceResponseBytes
            && isDeviceUrl(finalDescriptionUrl, deviceAddress);
        reply->deleteLater();
        if (!ok || generation != m_discoveryGeneration || !m_discovering)
            return;
        // Parse the device description XML for name and control URL
        QXmlStreamReader xml(data);
        QString name;
        QString serviceType;
        QString controlUrl;
        while (!xml.atEnd())
        {
            xml.readNext();
            if (!xml.isStartElement())
                continue;
            const QStringView element = xml.name();
            if (element == QStringLiteral("friendlyName"))
                name = xml.readElementText();
            else if (element == QStringLiteral("serviceType"))
                serviceType = xml.readElementText();
            else if (element == QStringLiteral("controlURL"))
            {
                const QString value = xml.readElementText();
                // Only capture the AVTransport service control URL
                if (serviceType.contains(QStringLiteral("AVTransport")))
                    controlUrl = value;
            }
        }
        if (xml.hasError() || name.isEmpty() || controlUrl.isEmpty())
            return;
        const QUrl resolved = finalDescriptionUrl.resolved(QUrl(controlUrl));
        if (!isDeviceUrl(resolved, deviceAddress) || m_devices.size() >= MaximumDeviceCandidates)
            return;
        // Avoid adding a duplicate device
        for (const QVariant& value : std::as_const(m_devices))
        {
            if (value.toMap().value(QStringLiteral("controlUrl")).toString() == resolved.toString())
                return;
        }
        m_devices.append(QVariantMap{{QStringLiteral("name"), name.left(256)},
                                     {QStringLiteral("controlUrl"), resolved.toString()},
                                     {QStringLiteral("descriptionUrl"), finalDescriptionUrl.toString()}});
        Q_EMIT devicesChanged();
    });
}

/**
 * @brief Sends a SOAP SetAVTransportURI to the selected device, then starts playback.
 * @param deviceIndex Index of the target device in the devices list.
 * @param mediaUrl    HTTP or HTTPS URL of the media content to cast.
 */
void CastService::castUrl(int deviceIndex, const QString& mediaUrl)
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()))
        return;
    const QUrl media(mediaUrl);
    // Only HTTP/HTTPS URLs are supported for this casting implementation
    if (!media.isValid() || media.host().isEmpty()
        || (media.scheme() != QStringLiteral("http") && media.scheme() != QStringLiteral("https")))
    {
        setStatusMessage(tr("This first casting release supports direct HTTP and HTTPS media URLs only"));
        return;
    }
    const QUrl controlUrl(m_devices.at(deviceIndex).toMap().value(QStringLiteral("controlUrl")).toString());
    // Build the SOAP SetAVTransportURI request body
    const QString escaped = media.toString(QUrl::FullyEncoded).toHtmlEscaped();
    const QByteArray body = QStringLiteral(
        "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>"
        "<u:SetAVTransportURI xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"><InstanceID>0</InstanceID>"
        "<CurrentURI>%1</CurrentURI><CurrentURIMetaData></CurrentURIMetaData></u:SetAVTransportURI>"
        "</s:Body></s:Envelope>").arg(escaped).toUtf8();
    QNetworkRequest request(controlUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/xml; charset=\"utf-8\""));
    request.setRawHeader("SOAPACTION", "\"urn:schemas-upnp-org:service:AVTransport:1#SetAVTransportURI\"");
    request.setTransferTimeout(DeviceRequestTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    QNetworkReply* reply = m_network.post(request, body);
    boundDeviceReply(reply);
    // On success, send the Play command to actually start playback
    connect(reply, &QNetworkReply::finished, this, [this, reply, controlUrl] {
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString error = reply->errorString();
        reply->deleteLater();
        if (!ok)
        {
            setStatusMessage(tr("Could not send media to the device: %1").arg(error));
            return;
        }
        sendPlay(controlUrl);
    });
}

/**
 * @brief Sends a SOAP Play command to the device to start media playback.
 * @param controlUrl The device's AVTransport control endpoint URL.
 */
void CastService::sendPlay(const QUrl& controlUrl)
{
    // Build the SOAP Play request body
    const QByteArray body = QByteArrayLiteral(
        "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>"
        "<u:Play xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"><InstanceID>0</InstanceID><Speed>1</Speed>"
        "</u:Play></s:Body></s:Envelope>");
    QNetworkRequest request(controlUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/xml; charset=\"utf-8\""));
    request.setRawHeader("SOAPACTION", "\"urn:schemas-upnp-org:service:AVTransport:1#Play\"");
    request.setTransferTimeout(DeviceRequestTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    QNetworkReply* reply = m_network.post(request, body);
    boundDeviceReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString error = reply->errorString();
        reply->deleteLater();
        setStatusMessage(ok ? tr("Playback started on the device")
                            : tr("The device could not start playback: %1").arg(error));
    });
}

/**
 * @brief Sets the discovering state and emits discoveringChanged() if different.
 * @param discovering New discovering state value.
 */
void CastService::setDiscovering(bool discovering)
{
    if (m_discovering == discovering)
        return;
    m_discovering = discovering;
    Q_EMIT discoveringChanged();
}

/**
 * @brief Sets the status message and emits statusMessageChanged() if different.
 * @param message New status text.
 */
void CastService::setStatusMessage(const QString& message)
{
    if (m_statusMessage == message)
        return;
    m_statusMessage = message;
    Q_EMIT statusMessageChanged();
}
