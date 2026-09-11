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

#include "services/FileService.h"

#include "utils/MediaUtils.h"
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QSet>
#include <QShortcut>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>

FileService::FileService(QObject* parent)
    : QObject(parent)
{}

FileService::~FileService()
{
    delete m_mediaDialog.data();
}

bool FileService::mediaDialogOpen() const
{
    return m_mediaDialog && m_mediaDialog->isVisible();
}

QString FileService::normalizeMediaPath(const QString& input) const
{
    const QFileInfo original(input);
    if (original.isFile() && original.isReadable())
        return original.absoluteFilePath();
    QString candidate = input.trimmed();
    if (candidate.size() >= 2 && candidate.front() == QLatin1Char('"') && candidate.back() == QLatin1Char('"'))
        candidate = candidate.mid(1, candidate.size() - 2);
    if (candidate.isEmpty())
        return {};
    const QUrl url = QUrl::fromUserInput(candidate, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (url.isLocalFile())
    {
        const QFileInfo file(url.toLocalFile());
        return file.isFile() && file.isReadable() ? file.absoluteFilePath() : QString();
    }
    return isSupportedUrl(url.toString()) ? url.toString(QUrl::FullyEncoded) : QString();
}

void FileService::openMediaDialog(bool append)
{
    if (mediaDialogOpen())
    {
        m_mediaDialog->raise();
        m_mediaDialog->activateWindow();
        return;
    }
    delete m_mediaDialog.data();
    auto* dialog = new QDialog;
    m_mediaDialog = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(append ? tr("Add Media") : tr("Open Media"));
    dialog->setWindowIcon(QApplication::windowIcon());
    dialog->setWindowModality(Qt::WindowModal);
    dialog->resize(620, 360);
    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    auto* label = new QLabel(tr("Paths or URLs"), dialog);
    auto* pathsInput = new QPlainTextEdit(dialog);
    pathsInput->setPlaceholderText(tr("Path or URL"));
    pathsInput->setAccessibleName(tr("Media paths or URLs"));
    pathsInput->setTabChangesFocus(true);
    pathsInput->setStyleSheet(QStringLiteral("QPlainTextEdit { border: 1px solid palette(mid); border-radius: 6px; padding: 8px; }"));
    label->setBuddy(pathsInput);
    layout->addWidget(label);
    layout->addWidget(pathsInput, 1);
    auto* feedback = new QLabel(dialog);
    feedback->setTextFormat(Qt::PlainText);
    feedback->setWordWrap(true);
    layout->addWidget(feedback);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, dialog);
    auto* openButton = buttons->button(QDialogButtonBox::Open);
    openButton->setText(append ? tr("Add") : tr("Open"));
    openButton->setEnabled(false);
    auto* browse = buttons->addButton(tr("Browse..."), QDialogButtonBox::ActionRole);
    browse->setIcon(dialog->style()->standardIcon(QStyle::SP_DirOpenIcon));
    layout->addWidget(buttons);
    connect(pathsInput, &QPlainTextEdit::textChanged, dialog, [pathsInput, openButton, feedback] {
        openButton->setEnabled(!pathsInput->toPlainText().trimmed().isEmpty());
        feedback->clear();
    });
    connect(browse, &QPushButton::clicked, this, [this, dialog, pathsInput] {
        const QStringList files = QFileDialog::getOpenFileNames(dialog, tr("Browse Media"), {},
            mediaNameFilters().join(QStringLiteral(";;")), nullptr, QFileDialog::DontUseNativeDialog);
        if (!files.isEmpty())
            pathsInput->appendPlainText(files.join(QLatin1Char('\n')));
    });
    connect(buttons, &QDialogButtonBox::accepted, this, [this, dialog, pathsInput, feedback, append] {
        QStringList paths;
        const auto entries = pathsInput->toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString& entry : entries)
        {
            if (entry.trimmed().isEmpty())
                continue;
            const QString path = normalizeMediaPath(entry);
            if (path.isEmpty())
            {
                feedback->setText(tr("Unavailable file or unsupported URL: %1").arg(entry.trimmed()));
                return;
            }
            paths.append(path);
        }
        if (paths.isEmpty())
            return;
        dialog->accept();
        Q_EMIT mediaChosen(paths, append);
    });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::finished, this, [this] { Q_EMIT mediaDialogOpenChanged(); });
    auto* submit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), dialog);
    connect(submit, &QShortcut::activated, openButton, &QPushButton::click);
    QWindow* parentWindow = QGuiApplication::focusWindow();
    dialog->winId();
    if (parentWindow)
    {
        dialog->windowHandle()->setTransientParent(parentWindow);
        if (parentWindow->screen())
        {
            const QRect available = parentWindow->screen()->availableGeometry();
            dialog->resize(dialog->size().boundedTo(available.size() * 0.9));
            dialog->move(available.center() - dialog->rect().center());
        }
    }
    dialog->show();
    pathsInput->setFocus();
    Q_EMIT mediaDialogOpenChanged();
}

QStringList FileService::mediaNameFilters() const
{
    return {tr("Media files (*.mkv *.mp4 *.webm *.avi *.mov *.mp3 *.flac *.wav *.png *.jpg *.m3u *.m3u8)"),
            tr("All files (*)")};
}

QStringList FileService::subtitleNameFilters() const
{
    return {tr("Subtitle files (*.srt *.ass *.ssa *.vtt *.sub *.idx *.sup)"), tr("All files (*)")};
}

QStringList FileService::audioNameFilters() const
{
    return {tr("Audio files (*.aac *.ac3 *.flac *.m4a *.mp3 *.ogg *.opus *.wav)"), tr("All files (*)")};
}

QStringList FileService::urlsToPaths(const QList<QUrl>& urls) const
{
    QStringList paths;
    for (const QUrl& url : urls)
    {
        paths.append(MediaUtils::urlToMpvPath(url));
    }
    return paths;
}

QStringList FileService::listMediaFiles(const QUrl& folder) const
{
    return MediaUtils::listMediaFiles(folder.toLocalFile());
}

bool FileService::isSubtitle(const QString& path) const
{
    return MediaUtils::isSubtitleFile(path);
}

bool FileService::isMedia(const QString& path) const
{
    return MediaUtils::isMediaFile(path);
}

QString FileService::displayName(const QString& path) const
{
    return MediaUtils::displayNameForPath(path);
}

QString FileService::iconNameForPath(const QString& path) const
{
    if (path.startsWith(QLatin1String("http://")) || path.startsWith(QLatin1String("https://")))
    {
        return QStringLiteral("cine-globe-symbolic");
    }

    QFileInfo info(path);
    QString extension = info.suffix().toLower();

    if (MediaUtils::isLocalPath(path) && !info.exists())
    {
        return QStringLiteral("cine-warning-symbolic");
    }

    // Check if it's a directory
    if (info.isDir())
    {
        return QStringLiteral("cine-folder-symbolic");
    }

    if (extension == QStringLiteral("m3u") || extension == QStringLiteral("m3u8"))
    {
        return QStringLiteral("cine-playlist-m3u-symbolic");
    }

    // Video
    static const QStringList videoExts = {QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("webm"),
                                          QStringLiteral("avi"), QStringLiteral("mov"), QStringLiteral("flv"),
                                          QStringLiteral("wmv"), QStringLiteral("m4v")};
    if (videoExts.contains(extension))
    {
        return QStringLiteral("cine-video-x-generic-symbolic");
    }

    // Audio
    static const QStringList audioExts = {QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("wav"),
                                          QStringLiteral("ogg"), QStringLiteral("opus"), QStringLiteral("m4a"),
                                          QStringLiteral("aac"), QStringLiteral("ac3")};
    if (audioExts.contains(extension))
    {
        return QStringLiteral("cine-audio-x-generic-symbolic");
    }

    // Image
    static const QStringList imageExts = {QStringLiteral("png"),  QStringLiteral("jpg"), QStringLiteral("jpeg"),
                                          QStringLiteral("webp"), QStringLiteral("gif"), QStringLiteral("bmp")};
    if (imageExts.contains(extension))
    {
        return QStringLiteral("cine-image-x-generic-symbolic");
    }

    return QStringLiteral("cine-applications-multimedia-symbolic");
}

bool FileService::isSupportedUrl(const QString& input) const
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
    {
        return false;
    }

    const QFileInfo localInfo(trimmed);
    if (localInfo.exists())
    {
        return localInfo.isDir() || MediaUtils::isMediaFile(trimmed) || MediaUtils::isSubtitleFile(trimmed);
    }

    const QUrl url = QUrl::fromUserInput(trimmed);
    if (url.isLocalFile())
    {
        const QString localPath = url.toLocalFile();
        const QFileInfo info(localPath);
        return info.exists()
               && (info.isDir() || MediaUtils::isMediaFile(localPath) || MediaUtils::isSubtitleFile(localPath));
    }

    static const QSet<QString> supportedSchemes = {
        QStringLiteral("http"),  QStringLiteral("https"), QStringLiteral("ftp"),       QStringLiteral("ftps"),
        QStringLiteral("sftp"),  QStringLiteral("smb"),   QStringLiteral("rtsp"),      QStringLiteral("rtmp"),
        QStringLiteral("rtmps"), QStringLiteral("udp"),   QStringLiteral("tcp"),       QStringLiteral("srt"),
        QStringLiteral("rist"),  QStringLiteral("mms"),   QStringLiteral("mmst"),      QStringLiteral("mmsh"),
        QStringLiteral("rtp"),   QStringLiteral("ytdl"),  QStringLiteral("ytdl+http"), QStringLiteral("ytdl+https")};

    return url.isValid() && supportedSchemes.contains(url.scheme().toLower()) && !url.host().isEmpty();
}

QString FileService::clipboardText() const
{
    const QClipboard* clipboard = QGuiApplication::clipboard();
    return clipboard ? clipboard->text().trimmed() : QString();
}

void FileService::openLocation(const QString& path) const
{
    if (path.trimmed().isEmpty())
    {
        return;
    }

    const QUrl url = QUrl::fromUserInput(path);
    if (!url.isLocalFile() && !url.scheme().isEmpty())
    {
        const QString scheme = url.scheme().toLower();
        if (url.isValid() && !url.host().isEmpty()
            && (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")))
            QDesktopServices::openUrl(url);
        return;
    }

    QFileInfo info(path);
    if (!info.exists() && url.isLocalFile())
    {
        info.setFile(url.toLocalFile());
    }
    if (!info.exists())
    {
        return;
    }

    const QString folder = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(folder)));
}
