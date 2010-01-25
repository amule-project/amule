//
// This file is part of the aMule Project.
//
// Copyright (c) 2010 Werner Mahr (Vollstrecker) <amule@vollstreckernet.de>
//
// Any parts of this program contributed by third-party developers are copyrighted
// by their respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "plasma-engine-plasmamule.h"

#include <kdebug.h>
#include <knotification.h>
#include <plasma/datacontainer.h>
#include <QDir>
#include <QFile>
#include <QTimer>

PlasmaMuleEngine::PlasmaMuleEngine (QObject* parent, const QVariantList& args)
	: Plasma::DataEngine (parent, args)
{
	Q_UNUSED (args)
	setMinimumPollingInterval (0);
}

bool PlasmaMuleEngine::sourceRequestEvent (const QString &name)
{
	return updateSourceEvent (name);
}

QStringList PlasmaMuleEngine::sources() const
{
	return QStringList() << "os_active"
		<< "config_found"
		<< "ed2k_state"
		<< "ed2k_server_name"
		<< "ed2k_server_ip"
		<< "ed2k_server_port"
		<< "ed2k_id_high_low"
		<< "kad_status"
		<< "down_speed"
		<< "up_speed"
		<< "clients_in_up_queue"
		<< "shared_files_count"
		<< "nickname"
		<< "total_bytes_downloaded"
		<< "total_bytes_uploaded"
		<< "version"
		<< "session_bytes_downloaded"
		<< "session_bytes_uploaded"
		<< "uptime";
}

void PlasmaMuleEngine::init ()
{
	QDir Dir;
	QStringList tempIncomingDirs;
	QStringList cleanedIncomingDirs;
	QStringList::const_iterator constIterator;
	QString Home = Dir.homePath();

	if (!m_timer)
	{
		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
		timer->start(60000);
		m_timer = TRUE;
	}
	if (!Home.endsWith("/"))
	{
		Home += "/";
	}
	QFile config_file (Home + ".aMule/amule.conf");

	if (!config_file.open (QIODevice::ReadOnly | QIODevice::Text))
	{
		setData(I18N_NOOP ("config_found"), FALSE);
		return;
	}

	QTextStream in (&config_file);
	while (!in.atEnd ())
	{
		QString line = in.readLine ();
		if (line.startsWith ("OnlineSignature="))
		{
			if (line.remove (0,line.indexOf ("=")+1) == "1")
			{
				m_OSActive = TRUE;
			} else {
				m_OSActive = FALSE;
			}
			setData(I18N_NOOP ("os_active"), m_OSActive);
		} else if (line.contains ("OSDirectory"))
		{
			m_OSFile = line.remove (0,line.indexOf ("=")+1) + "amulesig.dat";
		} else if (line.contains ("Incoming"))
		{
			if (!tempIncomingDirs.contains(line.remove (0,line.indexOf ("=")+1)))
			{
				tempIncomingDirs.append(line.remove (0,line.indexOf ("=")+1));
			}
		}
        }

	if (m_OSActive && !m_dirwatcher.contains(m_OSFile))
	{
		kDebug() << "Registering: " << m_OSFile << " for monitoring";
		m_dirwatcher.addFile (m_OSFile);
		connect (&m_dirwatcher, SIGNAL (dirty (const QString &)), SLOT (file_changed (const QString&)));
		connect (&m_dirwatcher, SIGNAL (created (const QString &)), SLOT (new_file (const QString&)));
	}

	for (constIterator = tempIncomingDirs.constBegin(); constIterator != tempIncomingDirs.constEnd(); constIterator++)
	{
		if (!m_dirwatcher.contains(*constIterator))
		{
			kDebug() << "Registering: " << *constIterator << " for monitoring";
			cleanedIncomingDirs.append (*constIterator);
			m_dirwatcher.addDir (*constIterator, KDirWatch::WatchFiles);
		} else {
			cleanedIncomingDirs.append (*constIterator);
		}
	}

	for (constIterator = m_incoming_dirs.constBegin(); constIterator != m_incoming_dirs.constEnd(); constIterator++)
	{
		if (!cleanedIncomingDirs.contains (*constIterator))
		{
			kDebug() << "Removing " << *constIterator << " from monitored dirs";
			m_dirwatcher.removeDir (*constIterator);
		}
	}

	m_incoming_dirs = cleanedIncomingDirs;
	config_file.close ();
	setName("plasmamule");
	setData(I18N_NOOP ("config_found"), TRUE);
}

void PlasmaMuleEngine::file_changed (const QString &path)
{
	if (path == m_OSFile)
	{
		kDebug() << "Rereading " << path;
		updateSourceEvent ("dummy");
	}
}

void PlasmaMuleEngine::new_file (const QString &path)
{
	kDebug() << "File " << path << "was created";
	KNotification::event(KNotification::Notification, QString("Finished Download of %1").arg(path));
}

void PlasmaMuleEngine::timeout()
{
	init();
	scheduleSourcesUpdated();
}

bool PlasmaMuleEngine::updateSourceEvent(const QString &name)
{
	Q_UNUSED (name)

	QFile file (m_OSFile);
	if (file.open (QIODevice::ReadOnly | QIODevice::Text) && m_OSActive)
	{
		QTextStream in (&file);
		setData(I18N_NOOP("ed2k_state"), in.readLine().toInt());
		setData(I18N_NOOP("ed2k_server_name"), in.readLine());
		setData(I18N_NOOP("ed2k_server_ip"), in.readLine());
		setData(I18N_NOOP("ed2k_server_port"), in.readLine().toInt());
		setData(I18N_NOOP("ed2k_id_high_low"), in.readLine());
		setData(I18N_NOOP("kad_status"), in.readLine().toInt());
		setData(I18N_NOOP("down_speed"), in.readLine().toDouble());
		setData(I18N_NOOP("up_speed"), in.readLine().toDouble());
		setData(I18N_NOOP("clients_in_up_queue"), in.readLine().toInt());
		setData(I18N_NOOP("shared_files_count"), in.readLine().toInt());
		setData(I18N_NOOP("nickname"), in.readLine());
		setData(I18N_NOOP("total_bytes_downloaded"), in.readLine().toLongLong());
		setData(I18N_NOOP("total_bytes_uploaded"), in.readLine().toLongLong());
		setData(I18N_NOOP("version"), in.readLine());
		setData(I18N_NOOP("session_bytes_downloaded"), in.readLine().toLongLong());
		setData(I18N_NOOP("session_bytes_uploaded"), in.readLine().toLongLong());
		setData(I18N_NOOP("uptime"), in.readLine().toInt());
		file.close();
		scheduleSourcesUpdated();
		return true;
	} else {
		return false;
	}
}

K_EXPORT_PLASMA_DATAENGINE(plasmamule, PlasmaMuleEngine)

#include "plasma-engine-plasmamule.moc"
