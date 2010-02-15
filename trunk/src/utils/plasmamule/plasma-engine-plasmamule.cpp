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

#include <QtDBus/QDBusInterface>
#include <QDir>
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
	return QStringList() << "categories"
		<< "clients_in_up_queue"
		<< "config_found"
		<< "dbus_registerred"
		<< "down_speed"
		<< "ed2k_state"
		<< "ed2k_server_name"
		<< "ed2k_server_ip"
		<< "ed2k_server_port"
		<< "ed2k_id_high_low"
		<< "kad_status"
		<< "nickname"
		<< "os_active"
		<< "session_bytes_downloaded"
		<< "session_bytes_uploaded"
		<< "shared_files_count"
		<< "total_bytes_downloaded"
		<< "total_bytes_uploaded"
		<< "up_speed"
		<< "uptime"
		<< "version";
}

void PlasmaMuleEngine::init ()
{
	Home = QDir::homePath();

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timer->start(60000);
	m_timer = TRUE;
	setData(I18N_NOOP("uptime"), 0);

	if (!Home.endsWith("/"))
	{
		Home += "/";
	}

	regDbus();
	initVals();
}

void PlasmaMuleEngine::regDbus ()
{
	new EngineAdaptor(this);
	QDBusConnection dbus = QDBusConnection::sessionBus();
	dbus.registerObject("/Link", this);
	kDebug() << "Registerred dbus: " << dbus.registerService("org.amule.engine");
}

void PlasmaMuleEngine::engine_add_link (const QString &link, const int &category)
{
	kDebug() << "Received Link " << link << " with cat " << category;

	QString link_to_write;

	if (link.startsWith("ed2k:") || link.startsWith("magnet:"))
	{
		link_to_write = link;
	} else {
		KNotification::event(KNotification::Notification, QString("%1 can't be handled by now.").arg(link));
		return;
	}

	if (category > 0)
	{
		link_to_write.append(QString(":%1").arg(category));
	}

	QFile link_file (Home + ".aMule/ED2KLinks");

	if (link_file.exists() && !link_file.open (QIODevice::WriteOnly | QIODevice::Append))
	{
		KNotification::event(KNotification::Notification, QString("Problem opening %1 for writing").arg(link_file.fileName()));
		return;
	}

	QTextStream out (&link_file);
	out << link_to_write << "\n";
	link_file.close();
	KNotification::event(KNotification::Notification, QString("Downloading %1").arg(link));
}

void PlasmaMuleEngine::initVals ()
{
	QStringList categories;
	QStringList tempIncomingDirs;
	QStringList cleanedIncomingDirs;
	QStringList::const_iterator constIterator;

	QFile config_file (Home + ".aMule/amule.conf");

	if (!config_file.open (QIODevice::ReadOnly | QIODevice::Text))
	{
		setData(I18N_NOOP ("config_found"), FALSE);
		return;
	}

	categories.append ("default");

	QTextStream in (&config_file);
	while (!in.atEnd())
	{
		QString line = in.readLine();
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
			m_OSFile.setFileName(line.remove (0,line.indexOf ("=")+1) + "amulesig.dat");
		} else if (line.contains ("Incoming"))
		{
			if (!tempIncomingDirs.contains(line.remove (0,line.indexOf ("=")+1)))
			{
				tempIncomingDirs.append(line.remove (0,line.indexOf ("=")+1));
			}
		} else if (line.startsWith ("Title"))
		{
			categories.append (line.remove (0,line.indexOf ("=")+1));
		}
        }

	setData(I18N_NOOP ("categories"), categories);

	if (m_OSActive && !m_dirwatcher.contains(m_OSFile.fileName()))
	{
		kDebug() << "Registering: " << m_OSFile.fileName() << " for monitoring";
		m_dirwatcher.addFile (m_OSFile.fileName());
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
	scheduleSourcesUpdated();
}

void PlasmaMuleEngine::file_changed (const QString &path)
{
	if (path == m_OSFile.fileName())
	{
		kDebug() << "Rereading " << path;
		updateSourceEvent ("dummy");
	}
}

void PlasmaMuleEngine::new_file (const QString &path)
{
	if (path != m_OSFile.fileName())
	{
		kDebug() << "File " << path << "was created";
		KNotification::event(KNotification::Notification, QString("Finished Download of %1").arg(path));
	}
}

void PlasmaMuleEngine::timeout()
{
	initVals();
}

bool PlasmaMuleEngine::updateSourceEvent(const QString &name)
{
	Q_UNUSED (name)

	if (m_OSFile.open (QIODevice::ReadOnly | QIODevice::Text) && m_OSActive)
	{
		QTextStream in (&m_OSFile);
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
		m_OSFile.close();
		scheduleSourcesUpdated();
		return true;
	} else {
		return false;
	}
}

K_EXPORT_PLASMA_DATAENGINE(plasmamule, PlasmaMuleEngine)

EngineAdaptor::EngineAdaptor(QObject *parent): QDBusAbstractAdaptor(parent)
{
	setAutoRelaySignals(true);
}

EngineAdaptor::~EngineAdaptor()
{
}

void EngineAdaptor::engine_add_link(const QString &link, const int &category)
{
	QMetaObject::invokeMethod(parent(), "engine_add_link", Q_ARG(QString, link), Q_ARG(int, category));
}

#include "plasma-engine-plasmamule.moc"
