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

#include "player/IpcProtocol.h"

#include <QJsonDocument>
#include <QProcess>

#include <algorithm>

/**
 * @brief Sets or replaces an observation in the session state.
 * @details Searches the observation list by ID. If an observation with the
 *          given ID already exists, it is replaced and the old value is returned.
 *          Otherwise, a new observation is appended.
 */
std::optional<IpcObservation> IpcSessionState::setObservation(qint64 id, const QString& property, bool stringValue)
{
    // Look for existing observation with the same ID
    const auto it = std::find_if(m_observations.begin(), m_observations.end(),
                                  [id](const IpcObservation& observation) {
                                      return observation.id == id;
                                  });
    std::optional<IpcObservation> replaced;
    if (it != m_observations.end())
    {
        // Replace existing observation and save the old value
        replaced = *it;
        *it = IpcObservation{id, property, stringValue};
    }
    else
    {
        // No existing observation, append a new one
        m_observations.append(IpcObservation{id, property, stringValue});
    }
    return replaced;
}

/**
 * @brief Removes an observation from the session state by ID.
 * @details Searches the observation list for the matching ID. If found,
 *          the observation is erased and returned to the caller.
 */
std::optional<IpcObservation> IpcSessionState::removeObservation(qint64 id)
{
    const auto it = std::find_if(m_observations.begin(), m_observations.end(),
                                  [id](const IpcObservation& observation) {
                                      return observation.id == id;
                                  });
    if (it == m_observations.end())
    {
        return std::nullopt;
    }

    const IpcObservation removed = *it;
    m_observations.erase(it);
    return removed;
}

/**
 * @brief Returns all observations for a specific mpv property.
 * @details Iterates the list and collects every observation whose property
 *          name matches the given string.
 */
QList<IpcObservation> IpcSessionState::observationsForProperty(const QString& property) const
{
    QList<IpcObservation> matches;
    for (const IpcObservation& observation : m_observations)
    {
        if (observation.property == property)
        {
            matches.append(observation);
        }
    }
    return matches;
}

/**
 * @brief Returns a copy of all active observations.
 */
QList<IpcObservation> IpcSessionState::observations() const
{
    return m_observations;
}

bool IpcSessionState::hasObservation(qint64 id) const
{
    return std::any_of(m_observations.cbegin(), m_observations.cend(),
                       [id](const IpcObservation& observation) { return observation.id == id; });
}

qsizetype IpcSessionState::observationCount() const
{
    return m_observations.size();
}

/**
 * @brief Parses a single line of IPC input into a structured result.
 * @details Handles two input formats:
 *          - JSON-RPC objects (starting with '{'): Extracts the command array,
 *            request ID, and validates required fields.
 *          - Space-separated text: Splits the line using QProcess::splitCommand
 *            and builds a command array from the resulting parts.
 *          Empty lines and lines starting with '#' are silently ignored.
 */
IpcParsedLine IpcProtocol::parseLine(const QByteArray& line)
{
    IpcParsedLine parsed;
    const QByteArray trimmed = line.trimmed();

    // Skip empty lines and comment lines
    if (trimmed.isEmpty() || trimmed.startsWith('#'))
    {
        return parsed;
    }

    if (trimmed.startsWith('{'))
    {
        // JSON-RPC format
        parsed.json = true;
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(trimmed, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            parsed.kind = IpcParsedLine::Kind::Error;
            parsed.error = QStringLiteral("invalid parameter");
            return parsed;
        }

        const QJsonObject object = document.object();
        parsed.requestId = object.contains(QStringLiteral("request_id"))
            ? object.value(QStringLiteral("request_id"))
            : QJsonValue(0);
        const QJsonValue commandValue = object.value(QStringLiteral("command"));
        if (!commandValue.isArray())
        {
            parsed.kind = IpcParsedLine::Kind::Error;
            parsed.error = QStringLiteral("invalid parameter");
            return parsed;
        }

        // Validate that the command array has a non-empty string as the first element
        parsed.command = commandValue.toArray();
        if (parsed.command.isEmpty() || !parsed.command.at(0).isString()
            || parsed.command.at(0).toString().isEmpty())
        {
            parsed.kind = IpcParsedLine::Kind::Error;
            parsed.error = QStringLiteral("invalid parameter");
            return parsed;
        }
    }
    else
    {
        // Space-separated text format (mpv-style)
        const QStringList parts = QProcess::splitCommand(QString::fromUtf8(trimmed));
        if (parts.isEmpty())
        {
            return parsed;
        }
        for (const QString& part : parts)
        {
            parsed.command.append(part);
        }
    }

    parsed.kind = IpcParsedLine::Kind::Command;
    return parsed;
}

/**
 * @brief Converts a JSON command array to a flat string list.
 * @details Each JSON value is converted using type-specific rules:
 *          - Strings are passed through as-is.
 *          - Bools become "yes" or "no".
 *          - Numbers are formatted with high precision.
 *          - Objects and arrays are compact-JSON serialized.
 *          - All other types become "null".
 */
QStringList IpcProtocol::commandArguments(const QJsonArray& command)
{
    QStringList arguments;
    arguments.reserve(command.size());
    for (const QJsonValue& value : command)
    {
        if (value.isString())
        {
            arguments.append(value.toString());
        }
        else if (value.isBool())
        {
            arguments.append(value.toBool() ? QStringLiteral("yes") : QStringLiteral("no"));
        }
        else if (value.isDouble())
        {
            arguments.append(QString::number(value.toDouble(), 'g', 17));
        }
        else if (value.isObject())
        {
            // Serialize JSON objects as compact strings for mpv
            arguments.append(QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact)));
        }
        else if (value.isArray())
        {
            arguments.append(QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact)));
        }
        else
        {
            arguments.append(QStringLiteral("null"));
        }
    }
    return arguments;
}

/**
 * @brief Builds a JSON-RPC response string.
 * @details Constructs a JSON object with "error", "request_id", and optionally
 *          "data" fields, serializes it in compact form, and appends a newline.
 */
QByteArray IpcProtocol::response(const QJsonValue& requestId, const QString& error, const QJsonValue& data)
{
    QJsonObject object;
    object[QStringLiteral("error")] = error;
    object[QStringLiteral("request_id")] = requestId;
    if (!data.isUndefined())
    {
        object[QStringLiteral("data")] = data;
    }
    return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
}

/**
 * @brief Builds a JSON-RPC event notification string.
 * @details Merges the provided fields with an "event" field, serializes
 *          in compact form, and appends a newline.
 */
QByteArray IpcProtocol::event(const QString& eventName, const QJsonObject& fields)
{
    QJsonObject object = fields;
    object[QStringLiteral("event")] = eventName;
    return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
}

/**
 * @brief Converts a QVariant to a QJsonValue.
 * @details Handles invalid/null variants by returning QJsonValue::Null.
 *          QJsonDocument variants are unwrapped to their array or object form.
 *          All other types use QJsonValue::fromVariant.
 */
QJsonValue IpcProtocol::variantToJson(const QVariant& value)
{
    if (!value.isValid() || value.isNull())
    {
        return QJsonValue::Null;
    }

    // Unwrap QJsonDocument to preserve array vs object distinction
    if (value.metaType() == QMetaType::fromType<QJsonDocument>())
    {
        const QJsonDocument document = value.toJsonDocument();
        return document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object());
    }
    return QJsonValue::fromVariant(value);
}
