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

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QUdpSocket>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class CastService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool discovering READ discovering NOTIFY discoveringChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    /**
     * @brief Constructs a CastService with an optional parent.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit CastService(QObject* parent = nullptr);

    /**
     * @brief Returns the list of discovered DLNA devices.
     * @return QVariantList of device maps (name, controlUrl, descriptionUrl).
     */
    QVariantList devices() const;

    /**
     * @brief Returns whether a device discovery scan is in progress.
     * @return True if actively discovering devices.
     */
    bool discovering() const;

    /**
     * @brief Returns the last status message.
     * @return Status text (e.g. "Found 2 devices", or an error).
     */
    QString statusMessage() const;

    /**
     * @brief Sends an SSDP M-SEARCH to discover DLNA media renderers on the LAN.
     */
    Q_INVOKABLE void discover();

    /**
     * @brief Casts a media URL to a discovered device by index.
     * @param deviceIndex Index into the devices list.
     * @param mediaUrl    HTTP or HTTPS URL of the media to play.
     */
    Q_INVOKABLE void castUrl(int deviceIndex, const QString& mediaUrl);

Q_SIGNALS:
    /**
     * @brief Emitted when the device list changes (devices added or cleared).
     */
    void devicesChanged();

    /**
     * @brief Emitted when the discovering state changes.
     */
    void discoveringChanged();

    /**
     * @brief Emitted when the status message is updated.
     */
    void statusMessageChanged();

private:
    /**
     * @brief Processes incoming UDP datagrams from SSDP discovery responses.
     */
    void readDiscoveryResponses();

    /**
     * @brief Fetches and parses a device's XML description document.
     * @param descriptionUrl URL to the device's UPnP description XML.
     */
    void inspectDevice(const QUrl& descriptionUrl, const QHostAddress& deviceAddress, int generation);

    /**
     * @brief Sends a SOAP Play command to the device's AVTransport control URL.
     * @param controlUrl The device's AVTransport control endpoint.
     */
    void sendPlay(const QUrl& controlUrl);

    /**
     * @brief Sets the discovering state and emits discoveringChanged().
     * @param discovering New discovering state.
     */
    void setDiscovering(bool discovering);

    /**
     * @brief Sets the status message and emits statusMessageChanged().
     * @param message New status text.
     */
    void setStatusMessage(const QString& message);

    /** @brief UDP socket used for SSDP discovery multicast. */
    QUdpSocket m_socket;
    /** @brief Network manager for HTTP requests to device description URLs. */
    QNetworkAccessManager m_network;
    /** @brief List of discovered device maps. */
    QVariantList m_devices;
    /** @brief Set of already-discovered location URLs to avoid duplicates. */
    QSet<QString> m_locations;
    /** @brief True while an SSDP discovery scan is active. */
    bool m_discovering{false};
    /** @brief Identifies the active scan so stale network replies are ignored. */
    int m_discoveryGeneration{0};
    /** @brief Last user-facing status message. */
    QString m_statusMessage;
};
