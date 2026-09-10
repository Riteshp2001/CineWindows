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

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <optional>

/**
 * @brief Represents a parsed IPC command line.
 * @details Contains the parsed command array, request ID, optional error string,
 *          and a flag indicating whether the original input was JSON or
 *          space-separated text. Used as the return type of IpcProtocol::parseLine().
 */
struct IpcParsedLine
{
    enum class Kind
    {
        Ignored,   ///< Empty line or comment
        Command,   ///< Valid parsed command
        Error      ///< Parse failure
    };

    Kind kind{Kind::Ignored};   ///< Classification of the parsed line
    QJsonArray command;          ///< Parsed command array (first element is command name)
    QJsonValue requestId{0};     ///< Client-provided request ID for response correlation
    QString error;               ///< Error message (non-empty only when kind is Error)
    bool json{false};            ///< True if the input was in JSON-RPC format
};

/**
 * @brief Represents a client's observation of an mpv property.
 * @details Tracks the observation ID, property name, and whether values
 *          should be returned as strings. Used by IpcServer and ConsoleReader
 *          to manage per-client property subscriptions.
 */
struct IpcObservation
{
    qint64 id{0};              ///< Client-assigned observation identifier
    QString property;           ///< mpv property name being observed
    bool stringValue{false};    ///< True if values should be returned as strings

    bool operator==(const IpcObservation&) const = default;
};

/**
 * @brief Tracks observed properties for a single IPC client session.
 * @details Manages a list of IpcObservation entries and provides methods
 *          to add, remove, and query observations by ID or property name.
 */
class IpcSessionState
{
public:
    /**
     * @brief Sets or replaces an observation by ID.
     * @param id Client-scoped observation identifier.
     * @param property mpv property name to observe.
     * @param stringValue True to return values as strings.
     * @return The previous observation if the ID was already registered, otherwise std::nullopt.
     */
    std::optional<IpcObservation> setObservation(qint64 id, const QString& property, bool stringValue);

    /**
     * @brief Removes an observation by ID.
     * @param id The observation identifier to remove.
     * @return The removed observation if found, otherwise std::nullopt.
     */
    std::optional<IpcObservation> removeObservation(qint64 id);

    /**
     * @brief Returns all observations for a given mpv property name.
     * @param property The property name to look up.
     * @return List of matching observations (may be empty).
     */
    QList<IpcObservation> observationsForProperty(const QString& property) const;

    /**
     * @brief Returns all active observations in this session.
     * @return Full list of IpcObservation entries.
     */
    QList<IpcObservation> observations() const;

    /** @brief Returns whether an observation ID is already registered. */
    bool hasObservation(qint64 id) const;

    /** @brief Returns the number of active observations. */
    qsizetype observationCount() const;

private:
    QList<IpcObservation> m_observations; ///< Active property observations for this session
};

/**
 * @brief Utility class for IPC protocol serialization and parsing.
 * @details Provides static methods to parse newline-delimited JSON-RPC or
 *          space-separated commands, serialize responses and events, and
 *          convert between QVariant and QJsonValue. Follows the mpv IPC
 *          protocol conventions.
 */
class IpcProtocol final
{
public:
    /**
     * @brief Parses a single line of IPC input.
     * @param line Raw UTF-8 byte array from socket or stdin.
     * @return IpcParsedLine containing the parsed command or error details.
     */
    static IpcParsedLine parseLine(const QByteArray& line);

    /**
     * @brief Converts a JSON command array to an mpv-style string argument list.
     * @param command The JSON array command to convert.
     * @return String list suitable for mpv command dispatch.
     */
    static QStringList commandArguments(const QJsonArray& command);

    /**
     * @brief Serializes a JSON-RPC response.
     * @param requestId The request ID to correlate the response.
     * @param error Error string ("success" on success).
     * @param data Optional response data payload.
     * @return Newline-terminated JSON byte array.
     */
    static QByteArray response(const QJsonValue& requestId, const QString& error,
                               const QJsonValue& data = QJsonValue(QJsonValue::Undefined));

    /**
     * @brief Serializes a JSON-RPC event notification.
     * @param eventName The event name (e.g. "property-change", "file-loaded").
     * @param fields Additional event-specific fields to include.
     * @return Newline-terminated JSON byte array.
     */
    static QByteArray event(const QString& eventName, const QJsonObject& fields = {});

    /**
     * @brief Converts a QVariant to a QJsonValue.
     * @param value The QVariant to convert.
     * @return Equivalent QJsonValue, or QJsonValue::Null for invalid/null variants.
     */
    static QJsonValue variantToJson(const QVariant& value);
};
