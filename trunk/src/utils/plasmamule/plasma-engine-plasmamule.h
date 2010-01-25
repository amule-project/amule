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

#ifndef PLASMAMULENGINE_H
#define PLASMAMULENGINE_H

#include <plasma/dataengine.h>
#include <QStringList>
#include <kdirwatch.h>

class PlasmaMuleEngine : public Plasma::DataEngine
{
    // required since Plasma::DataEngine inherits QObject
    Q_OBJECT

	public:
		PlasmaMuleEngine(QObject* parent, const QVariantList& args);
		QStringList sources() const;
		void init ();

	public Q_SLOTS:
		void file_changed (const QString &path);
		void new_file (const QString &path);
		void timeout ();

	protected:
		bool sourceRequestEvent(const QString& name);
		bool updateSourceEvent(const QString& name);

	private:
		bool m_OSActive, m_timer;
		QString m_OSFile;
		KDirWatch m_dirwatcher;
		QStringList m_incoming_dirs;
};

#endif // PLASMAMULENGINE_H
