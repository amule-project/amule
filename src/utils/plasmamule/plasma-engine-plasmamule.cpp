//
// This file is part of the aMule Project.
//
// Copyright (c) 2010-2011 Werner Mahr (Vollstrecker) <amule@vollstreckernet.de>
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
#include "qt-emc.h"

#include <kdebug.h>
#include <knotification.h>

#include <kio/scheduler.h>

#include <plasma/datacontainer.h>

#include <QDir>
#include <QTimer>

#include <QtDBus/QDBusInterface>

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
	return QStringList() << "cat_dirs"
		<< "cat_names"
		<< "clients_in_up_queue"
		<< "config_found"
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

	m_debugChannel = KDebug::registerArea ("plasmamule-engine", 
#ifdef __DEBUG__
	true
#else
	false
#endif
	);
  
	regDbus();
	initVals();
}

void PlasmaMuleEngine::regDbus ()
{
	new EngineAdaptor(this);
	QDBusConnection dbus = QDBusConnection::sessionBus();
	dbus.registerObject("/Link", this);
	kDebug(m_debugChannel) << "Registerred dbus: " << dbus.registerService("org.amule.engine");
}

void PlasmaMuleEngine::downloadFinished (KIO::Job* job,const QByteArray& data)
{

	if (data.length() == 0)
	{
		KNotification::event(KNotification::Notification, QString("Download of %1 failed.").arg(job->queryMetaData("Name")));
		return;
	}

	kDebug(m_debugChannel) << QString("Finished download of %1").arg(job->queryMetaData("Name"));

	QString downloadFileName(QString("/tmp/plasmamule-download-%1.emulecollection").arg(qrand()));

	QFile downloadFile(downloadFileName);

	if (!downloadFile.open (QIODevice::WriteOnly | QIODevice::Append))
	{
		KNotification::event(KNotification::Notification, QString("%1 can't be written to temp-file.").arg(job->queryMetaData("Name")));
		return;
	}

	QDataStream out(&downloadFile);
	out.writeRawData(data, data.length());
	downloadFile.close();

	engine_add_link (downloadFileName, job->queryMetaData("Category").toInt(), job->queryMetaData("Name"));

	downloadFile.remove();
}

void PlasmaMuleEngine::engine_add_link (const QString &link, const int &category, const QString &printname)
{
	kDebug(m_debugChannel) << "Received Link " << link << " with cat " << category;

	QString link_to_write;

	
	if (link.startsWith("ed2k:") || link.startsWith("magnet:"))
	{
		link_to_write = link;

		if (category > 0)
		{
			link_to_write.append(QString(":%1").arg(category));
		}

		link_to_write.append("\n");
	} else if (link.contains(".emulecollection") && KUrl(link).isLocalFile())
	{
		qtEmc* collection = new qtEmc(link);
		if (collection->isValid())
		{
			QStringList links = collection->getLinks();
			for (QStringList::const_iterator constIterator = links.constBegin(); constIterator != links.constEnd(); constIterator++)
			{
				link_to_write.append(*constIterator);

				if (category > 0)
				{
					link_to_write.append(QString(":%1").arg(category));
				}

				link_to_write.append("\n");
			}
		} else {
			KNotification::event(KNotification::Notification, collection->getErrorMessage());
		}

		delete collection;
	} else {
		KIO::TransferJob *job = KIO::get(KUrl(link));
		job->addMetaData("Name", link);
		job->addMetaData("Category", QString(category));
		connect (job, SIGNAL(data(KIO::Job *, const QByteArray&)), this,
			SLOT(downloadFinished(KIO::Job *,const QByteArray&)));
		kDebug(m_debugChannel) << QString("Starting download of %1").arg(printname);
		return;
	}

	QFile link_file (Home + ".aMule/ED2KLinks");

	if (!link_file.open (QIODevice::WriteOnly | QIODevice::Append))
	{
		KNotification::event(KNotification::Notification, QString("Problem opening %1 for writing").arg(link_file.fileName()));
		return;
	}

	QTextStream out (&link_file);
	out << link_to_write;
	out.flush();
	link_file.close();

	KNotification::event(KNotification::Notification, QString("Downloading %1").arg(printname));

}

void PlasmaMuleEngine::initVals ()
{
	QStringList catDir;
	QStringList catName;
	QStringList tempIncomingDirs;
	QStringList cleanedIncomingDirs;
	QStringList::const_iterator constIterator;

	QFile config_file (Home + ".aMule/amule.conf");

	catName.append("Default");
	if (!config_file.open (QIODevice::ReadOnly | QIODevice::Text))
	{
		setData(I18N_NOOP ("config_found"), FALSE);
		return;
	}

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

			catDir.append(line.remove (0,line.indexOf ("=")+1));
		} else if (line.startsWith ("Title"))
		{
			catName.append(line.remove (0,line.indexOf ("=")+1));
		}
        }

	setData(I18N_NOOP ("cat_names"), catName);
	setData(I18N_NOOP ("cat_dirs"), catDir);

	if (m_OSActive && !m_dirwatcher.contains(m_OSFile.fileName()))
	{
		kDebug(m_debugChannel) << "Registering: " << m_OSFile.fileName() << " for monitoring";
		m_dirwatcher.addFile (m_OSFile.fileName());
		connect (&m_dirwatcher, SIGNAL (dirty (const QString &)), SLOT (file_changed (const QString&)));
		connect (&m_dirwatcher, SIGNAL (created (const QString &)), SLOT (new_file (const QString&)));
	}

	for (constIterator = tempIncomingDirs.constBegin(); constIterator != tempIncomingDirs.constEnd(); constIterator++)
	{
		if (!m_dirwatcher.contains(*constIterator))
		{
			kDebug(m_debugChannel) << "Registering: " << *constIterator << " for monitoring";
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
			kDebug(m_debugChannel) << "Removing " << *constIterator << " from monitored dirs";
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
		kDebug(m_debugChannel) << "Rereading " << path;
		updateSourceEvent ("dummy");
	}
}

void PlasmaMuleEngine::new_file (const QString &path)
{
	if (path != m_OSFile.fileName())
	{
		kDebug(m_debugChannel) << "File " << path << "was created";
		KNotification::event(KNotification::Notification, QString("Finished Download of %1").arg(path));
	} else {
		kDebug(m_debugChannel) << "Rereading " << path;
		updateSourceEvent ("dummy");
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
	QMetaObject::invokeMethod(parent(), "engine_add_link", Q_ARG(QString, link), Q_ARG(int, category), Q_ARG(QString, link));
}

#include "plasma-engine-plasmamule.moc"
