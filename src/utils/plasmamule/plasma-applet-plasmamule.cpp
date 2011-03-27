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
#include "plasma-applet-plasmamule.h"
#include "plasmamule-dbus.h"

#include <kaboutapplicationdialog.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kicon.h>
#include <kplugininfo.h>
#include <krun.h>
#include <kservice.h>

#include <plasma/svg.h>
#include <plasma/theme.h>
#include <plasma/version.h>

#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QSizeF>
#include <QStringList>

K_EXPORT_PLASMA_APPLET(plasma-applet-plasmamule, PlasmaMuleApplet)

PlasmaMuleApplet::PlasmaMuleApplet(QObject *parent, const QVariantList &args)
	: Plasma::Applet(parent, args),
	m_svg(this)
{
	QString path = __IMG_PATH__;
	path.append( "application-x-emulecollection.svg");
	m_svg.setImagePath(path);
	setBackgroundHints(TranslucentBackground);
	setMinimumSize(200, 200);
	setMaximumSize(300, 300);
	setAcceptDrops(TRUE);
	setHasConfigurationInterface(FALSE);
}

PlasmaMuleApplet::~PlasmaMuleApplet()
{
}
 
void PlasmaMuleApplet::init()
{
	m_debugChannel = KDebug::registerArea ("plasmamule-applet", 
#ifdef __DEBUG__
	true
#else
	false
#endif
	);
	connectToEngine();
}

void PlasmaMuleApplet::paintInterface(QPainter *painter,
	const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->setRenderHint(QPainter::Antialiasing);
	QPixmap pixmap;
	pixmap = m_svg.pixmap();
	QPixmap temp(pixmap.size());
	temp.fill(Qt::transparent);
	QPainter p(&temp);
	p.setCompositionMode(QPainter::CompositionMode_Source);
	p.drawPixmap(0, 0, pixmap);
	p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	p.fillRect(temp.rect(), QColor(0, 0, 0, 200));
	p.end();
	pixmap = temp;
	painter->drawPixmap(contentsRect, pixmap);
	painter->save();
	painter->setPen(Qt::red);
	QFont font = painter->font();
	font.setBold(true);
	painter->setFont(font);
	if (!m_config_found)
	{	
		painter->drawText(contentsRect, Qt::AlignCenter,
		"aMule not configured or installed");
	}
	else if (!m_os_active)
	{	
		painter->drawText(contentsRect, Qt::AlignCenter,
		"Online Signature disabled");
	} else if (m_uptime == 0)
	{
		painter->drawText(contentsRect, Qt::AlignCenter,
		"aMule is not running\n");
	} else {
		QString message;
		painter->setPen(Qt::black);
		int time, days, hours, minutes;
		QString runtime;
		days = m_uptime/86400;
		time = m_uptime%86400;
		hours=time/3600;
		time=time%3600;
		minutes=time/60;
		time=time%60;

		runtime = QString("%1:%2:%3").arg(hours, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0')).arg(time, 2, 10, QLatin1Char('0'));

		if (days)
		{
			if (days = 1)
			{
				runtime.prepend(QString("%1 day ").arg(days));
			} else {
				runtime.prepend(QString("%1 days ").arg(days));
			}
		}
		message = QString("%1 has been running for %2 \n\n").arg(m_version).arg(runtime);
		if (m_ed2k_state == 0 && m_kad_status == 0)
		{
			message.append(QString("%1 is not connected ").arg(m_nickname));
			message.append("but running\n\n");
		}
		else if (m_ed2k_state == 0 && m_kad_status != 0)
		{
			message.append(QString("%1 is connected to ").arg(m_nickname));
			if (m_kad_status == 1)
			{
				message.append("Kad: firewalled \n\n");
			} else {
				message.append("Kad: ok \n\n");
			}
		} else {
			message.append(QString("%1 is connected to %2 [%3:%4] with ").arg(m_nickname).arg(m_ed2k_server_name).arg(m_ed2k_server_ip).arg(m_ed2k_server_port));
			if (m_kad_status == 1)
			{
				if (m_ed2k_id_high_low == "H")
				{
					message.append("HighID | Kad: firewalled \n\n");
				} else {
					message.append("LowID | Kad: firewalled \n\n");
				}
			} else if (m_kad_status == 2)
			{
				if (m_ed2k_id_high_low == "H")
				{
					message.append("HighID | Kad: ok \n\n");
				} else {
					message.append("LowID | Kad: ok \n\n");
				}
			} else {
				if (m_ed2k_id_high_low == "H")
				{
					message.append("HighID | Kad: off \n\n");
				} else {
					message.append("LowID | Kad: off \n\n");
				}
			}
		}
		message.append(QString("Total Download: %1, Upload: %2\n\n").arg(calcSize(m_total_bytes_downloaded)).arg(calcSize(m_total_bytes_uploaded)));
		message.append(QString("Session Download: %1, Upload: %2\n\n").arg(calcSize(m_session_bytes_downloaded)).arg(calcSize(m_session_bytes_uploaded)));
		message.append(QString("Download: %L1 kB/s, Upload: %L2 kB/s\n\n").arg(m_down_speed, 0 , 'f', 1).arg(m_up_speed,0 ,'f', 1));
		QString files_unit;
		if (m_shared_files_count == 1)
		{
			files_unit = "file";
		} else {
			files_unit = "files";
		}
		message.append(QString("Sharing: %1 %2, Clients on queue: %3").arg(m_shared_files_count).arg(files_unit).arg(m_clients_in_up_queue));
		painter->drawText(contentsRect, Qt::TextDontClip | Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, message);

		if (painter->boundingRect(contentsRect, Qt::TextDontClip | Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap,
			 message).height() > contentsRect.height())
		{
			kDebug(m_debugChannel) << "Resizing";
			resize(painter->boundingRect(contentsRect, message).height()+(contentsRect.topLeft().y()*2),
				painter->boundingRect(contentsRect, message).height()+(contentsRect.topLeft().y()*2));
		}
	}
	painter->restore();
}

QString PlasmaMuleApplet::calcSize (qlonglong in_size)
{
	int unit=0;
	double size;
	QStringList units;
	units << "Bytes" << "kBs" << "MBs" << "GBs" << "TBs" << "PBs" << "BBs" << "ZBs" << "YBs";

	while (in_size >1023)
	{
		in_size /= 1024;
		unit++;
	}

	size = (in_size * 1024) / 1024;
	return QString("%L1 %2").arg(size, 0, 'f', 2).arg(units.at(unit));
}

void PlasmaMuleApplet::connectToEngine()
{
	m_aMuleEngine = dataEngine("plasmamule");
	m_aMuleEngine->connectAllSources(this, 0);
	connect(m_aMuleEngine, SIGNAL(sourceAdded(const QString&)), this, SLOT(onSourceAdded(const QString&)));
	connect(m_aMuleEngine, SIGNAL(sourceRemoved(const QString&)), this, SLOT(onSourceRemoved(const QString&)));
}

void PlasmaMuleApplet::onSourceAdded(const QString& source)
{
	kDebug(m_debugChannel) << "New Source: " << source << " added";
	m_aMuleEngine->connectSource(source, this, 0);
}

void PlasmaMuleApplet::onSourceRemoved(const QString& source)
{
	kDebug(m_debugChannel) << "Source: " << source << " removed";
	update();
}

void PlasmaMuleApplet::dataUpdated(const QString& source, const Plasma::DataEngine::Data &data)
{
	if (data.isEmpty())
	{
		return;
	}

	bool needs_update = FALSE;
	kDebug(m_debugChannel) << "Updating data" << data;
	if (data["cat_dirs"].toStringList() != m_catDirs && data.contains("cat_dirs"))
	{
		m_catDirs = data["cat_dirs"].toStringList();
	}
	if (data["cat_names"].toStringList() != m_catNames && data.contains("cat_names"))
	{
		m_catNames = data["cat_names"].toStringList();
	}
	if (data["os_active"].toBool() != m_os_active && data.contains("os_active"))
	{
		m_os_active = data["os_active"].toBool();
		needs_update = TRUE;
	}
	if (data["config_found"].toBool() != m_config_found && data.contains("config_found"))
	{
		m_config_found = data["config_found"].toBool();
		needs_update = TRUE;
	}
	if (data["ed2k_state"].toInt() != m_ed2k_state && data.contains("ed2k_state"))
	{
		m_ed2k_state = data["ed2k_state"].toInt();
		needs_update = TRUE;
	}
	if (data["ed2k_server_name"] != m_ed2k_server_name && data.contains("ed2k_server_name"))
	{
		m_ed2k_server_name = data["ed2k_server_name"].toString();
		needs_update = TRUE;
	}
	if (data["ed2k_server_ip"] != m_ed2k_server_ip && data.contains("ed2k_server_ip"))
	{
		m_ed2k_server_ip = data["ed2k_server_ip"].toString();
		needs_update = TRUE;
	}
	if (data["ed2k_server_port"].toInt() != m_ed2k_server_port && data.contains("ed2k_server_port"))
	{
		m_ed2k_server_port = data["ed2k_server_port"].toInt();
		needs_update = TRUE;
	}
	if (data["ed2k_id_high_low"] != m_ed2k_id_high_low && data.contains("ed2k_id_high_low"))
	{
		m_ed2k_id_high_low = data["ed2k_id_high_low"].toString();
		needs_update = TRUE;
	}
	if (data["kad_status"].toInt() != m_kad_status && data.contains("kad_status"))
	{
		m_kad_status = data["kad_status"].toInt();
		needs_update = TRUE;
	}
	if (data["down_speed"].toInt() != m_down_speed && data.contains("down_speed"))
	{
		m_down_speed = data["down_speed"].toDouble();
		needs_update = TRUE;
	}
	if (data["up_speed"].toDouble() != m_up_speed && data.contains("up_speed"))
	{
		m_up_speed = data["up_speed"].toDouble();
		needs_update = TRUE;
	}
	if (data["clients_in_up_queue"].toInt() != m_clients_in_up_queue && data.contains("clients_in_up_queue"))
	{
		m_clients_in_up_queue = data["clients_in_up_queue"].toInt();
		needs_update = TRUE;
	}
	if (data["shared_files_count"].toInt() != m_shared_files_count && data.contains("shared_files_count"))
	{
		m_shared_files_count = data["shared_files_count"].toInt();
		needs_update = TRUE;
	}
	if (data["nickname"] != m_nickname && data.contains("nickname"))
	{
		m_nickname = data["nickname"].toString();
		needs_update = TRUE;
	}
	if (data["total_bytes_downloaded"].toLongLong() != m_total_bytes_downloaded && data.contains("total_bytes_downloaded"))
	{
		m_total_bytes_downloaded = data["total_bytes_downloaded"].toLongLong();
		needs_update = TRUE;
	}
	if (data["total_bytes_uploaded"].toLongLong() != m_total_bytes_uploaded && data.contains("total_bytes_uploaded"))
	{
		m_total_bytes_uploaded = data["total_bytes_uploaded"].toLongLong();
		needs_update = TRUE;
	}
	if (data["version"] != m_version && data.contains("version"))
	{
		m_version = data["version"].toString();
		needs_update = TRUE;
	}
	if (data["session_bytes_downloaded"].toLongLong() != m_session_bytes_downloaded && data.contains("session_bytes_downloaded"))
	{
		m_session_bytes_downloaded = data["session_bytes_downloaded"].toLongLong();
		needs_update = TRUE;
	}
	if (data["session_bytes_uploaded"].toLongLong() != m_session_bytes_uploaded && data.contains("session_bytes_uploaded"))
	{
		m_session_bytes_uploaded = data["session_bytes_uploaded"].toLongLong();
		needs_update = TRUE;
	}
	if (data["uptime"].toInt() != m_uptime && data.contains("uptime"))
	{
		m_uptime = data["uptime"].toInt();
		needs_update = TRUE;
	}

	if (needs_update)
	{
		kDebug(m_debugChannel) << "Updating view";
		update();
	}
}

void PlasmaMuleApplet::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
	kDebug(m_debugChannel) << "Dragged Data detected " << event;
	if (event->mimeData()->hasUrls())
	{
		event->acceptProposedAction();
	}
}

void PlasmaMuleApplet::dropEvent(QGraphicsSceneDragDropEvent * event)
{
	QStringList::const_iterator constIterator;

	event->acceptProposedAction();

	QMenu *menu = new QMenu;

	if (m_catNames.count() == 1)
	{
		sendLinkToEngine (event->mimeData()->text(), 0, this, m_debugChannel);
	} else {
		for (constIterator = m_catNames.constBegin(); constIterator != m_catNames.constEnd(); constIterator++)
		{
			menu->addAction(*constIterator);
			if (constIterator != m_catNames.constEnd())
			{
				menu->addSeparator();
			}
		}

		QAction *cat_selection = menu->exec(QCursor::pos());
		if (cat_selection)
		{
				sendLinkToEngine (event->mimeData()->text(), m_catNames.indexOf(cat_selection->text()), this, m_debugChannel);
		}
	}

	delete menu;
}

void PlasmaMuleApplet::contextMenuEvent(QGraphicsSceneContextMenuEvent * event)
{
	QStringList::const_iterator constIterator;
	QMenu menu;

	if (m_catDirs.count() == 1)
	{
		menu.addAction (KIcon("folder"), QString("Default"));
	} else {
		QMenu *open_sub_menu = menu.addMenu("Open Incoming");
		for (constIterator = m_catNames.constBegin(); constIterator != m_catNames.constEnd(); constIterator++)
		{
			open_sub_menu->addAction(KIcon("folder"), "Folder for " + *constIterator);
			if (constIterator != m_catNames.constEnd())
			{
				open_sub_menu->addSeparator();
			}
		}
		menu.addSeparator();
	}

	QClipboard* clipboard = QApplication::clipboard();

	if (!clipboard->text(QClipboard::Clipboard).isEmpty() || !clipboard->text(QClipboard::Selection).isEmpty())
	{
		if (m_catDirs.count() == 1)
		{
			if (!clipboard->text(QClipboard::Clipboard).isEmpty())
			{
				menu.addAction(KIcon("arrow-down-double"), "Download Link from Clipboard");

				if (!clipboard->text(QClipboard::Selection).isEmpty())
				{
					menu.addSeparator();
				}
			}

			if (!clipboard->text(QClipboard::Selection).isEmpty())
			{
				menu.addAction(KIcon("arrow-down-double"), "Download Link from Selection");
			}
		} else 	{
			QMenu *download_sub_menu = menu.addMenu("Download Link");

			for (constIterator = m_catNames.constBegin(); constIterator != m_catNames.constEnd(); constIterator++)
			{
				if (!clipboard->text(QClipboard::Clipboard).isEmpty())
				{
					download_sub_menu->addAction(KIcon("arrow-down-double"), "Clipboard->" + *constIterator);

					if (!clipboard->text(QClipboard::Selection).isEmpty())
					{
						download_sub_menu->addSeparator();
					}
				}

				if (!clipboard->text(QClipboard::Selection).isEmpty())
				{
					download_sub_menu->addAction(KIcon("arrow-down-double"), "Selection->" + *constIterator);
				}

				if (constIterator != m_catNames.constEnd())
				{
				download_sub_menu->addSeparator();
				}
			}
		}
	}

	menu.addSeparator();
	menu.addAction(KIcon("documentinfo"), "About");

	QAction* selectedItem = menu.exec(QCursor::pos());
	if (selectedItem)
	{
		if (selectedItem->text() == "About")
		{
			KPluginInfo* service = new KPluginInfo (KService::serviceByDesktopName ("plasma-applet-plasmamule"));
			KAboutData* aboutData = new KAboutData (service->name().toUtf8(),
				service->name().toUtf8(),
				ki18n(service->pluginName().toUtf8()),
				service->version().toUtf8(),
				ki18n(service->comment().toUtf8()),
				KAboutData::License_GPL_V3,
				ki18n(QByteArray()),
				ki18n(QByteArray()),
				service->website().toLatin1(),
				service->email().toLatin1());
			aboutData->addAuthor (ki18n(service->author().toUtf8()),
				ki18n(QByteArray()),
				service->email().toLatin1(),
				service->website().toLatin1());
			aboutData->setTranslator(ki18nc ("NAME OF TRANSLATORS", "Your names"),
				ki18nc("EMAIL OF TRANSLATORS", "Your emails"));
			aboutData->setProgramIconName (service->icon());

			KAboutApplicationDialog* about = new KAboutApplicationDialog(aboutData, KAboutApplicationDialog::HideTranslators);
			about->exec();
		} else if (selectedItem->text().startsWith("Folder for"))
		{
			kDebug(m_debugChannel) << "Opening Folder " << m_catDirs.at(m_catNames.indexOf(selectedItem->text().remove("Folder for ")));
			KUrl url(m_catDirs.at(m_catNames.indexOf(selectedItem->text().remove("Folder for "))) + "/");
                	(void) new KRun( url, 0, true );
		} else if (selectedItem->text().startsWith("Download"))
		{
			if (selectedItem->text().remove("Download Link from ") == "Clipboard")
			{
				sendLinkToEngine (clipboard->text(QClipboard::Clipboard), 0, this, m_debugChannel);
			} else {
				sendLinkToEngine (clipboard->text(QClipboard::Selection), 0, this, m_debugChannel);
			}
		} else if (selectedItem->text().startsWith("Clipboard->"))
		{
			sendLinkToEngine (clipboard->text(QClipboard::Clipboard), m_catNames.indexOf(selectedItem->text().remove("Clipboard->")), this, m_debugChannel);
		} else if (selectedItem->text().startsWith("Selection->"))
		{
			sendLinkToEngine (clipboard->text(QClipboard::Selection), m_catNames.indexOf(selectedItem->text().remove("Selection->")), this, m_debugChannel);
		}
	}
}

#include "plasma-applet-plasmamule.moc"
