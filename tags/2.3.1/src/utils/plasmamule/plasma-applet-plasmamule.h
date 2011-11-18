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

#ifndef plasmamule_applet_h
#define plasmamule_applet_h

#include <plasma/applet.h>
#include <plasma/dataengine.h>

#include <QGraphicsSceneDragDropEvent>


class PlasmaMuleApplet : public Plasma::Applet
{
	Q_OBJECT
	public:
		PlasmaMuleApplet(QObject *parent, const QVariantList &args);
		~PlasmaMuleApplet();

		void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& contentsRect);
		void init();
		
	public Q_SLOTS:
		void onSourceAdded(const QString&);
		void onSourceRemoved(const QString&);
		void dataUpdated(const QString&, const Plasma::DataEngine::Data&);

	protected:
		void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		void dropEvent(QGraphicsSceneDragDropEvent *event);
		void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

	private:
		void connectToEngine();

		QString calcSize (qlonglong);
		QStringList m_catDirs, m_catNames;
		int m_ed2k_state, m_ed2k_server_port, m_kad_status, m_clients_in_up_queue, m_shared_files_count, m_uptime, m_debugChannel;
		QString  m_ed2k_id_high_low, m_ed2k_server_name, m_ed2k_server_ip, m_nickname, m_version;
		double m_down_speed, m_up_speed;
		qlonglong m_total_bytes_downloaded, m_total_bytes_uploaded, m_session_bytes_downloaded, m_session_bytes_uploaded;
		bool m_os_active, m_config_found;
		Plasma::Svg m_svg;
		Plasma::DataEngine* m_aMuleEngine;

	signals:
		void engine_add_link(QString, int);
};
 
#endif
