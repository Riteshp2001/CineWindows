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
 * Last modified: 2026-09-11
 * Modified by: Ritesh Pandit
 */

#pragma once

#include <QString>
#include <QtGlobal>

/** Owns the process-wide QtLogger configuration and shutdown lifecycle. */
class ApplicationLog final
{
public:
    ApplicationLog();
    ~ApplicationLog();

    Q_DISABLE_COPY_MOVE(ApplicationLog)

    /** @return Absolute path to the active rotating log file, or empty on fallback. */
    QString filePath() const;

private:
    QString m_filePath;
    bool m_configured{false};
};
