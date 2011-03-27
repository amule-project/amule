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

#ifndef PLASMAMULENGINE_H
#define PLASMAMULENGINE_H

#include <kdirwatch.h>

#include <kio/jobclasses.h>

#include <plasma/dataengine.h>

#include <QtDBus/QtDBus>
#include <QFile>
#include <QStringList>

class PlasmaMuleEngine : public Plasma::DataEngine
{

	Q_OBJECT

	public:
		PlasmaMuleEngine(QObject* parent, const QVariantList& args);
		QStringList sources() const;
		void init ();

	public Q_SLOTS:
		Q_SCRIPTABLE void engine_add_link (const QString &link, const int &category, const QString &printname);

	protected:
		bool sourceRequestEvent(const QString& name);
		bool updateSourceEvent(const QString& name);

	protected Q_SLOTS:
		void downloadFinished (KIO::Job *,const QByteArray&);
		void file_changed (const QString &path);
		void new_file (const QString &path);
		void timeout ();

	private:
		void initVals ();
		void regDbus ();

		bool m_OSActive, m_timer;

		int m_debugChannel;
		KDirWatch m_dirwatcher;
		QFile m_OSFile;
		QStringList m_incoming_dirs, downloadsNames;
		QString Home;
};

class EngineAdaptor: public QDBusAbstractAdaptor
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.amule.engine")
	Q_CLASSINFO("D-Bus Introspection", ""
	"  <interface name=\"org.amule.engine\" >\n"
	"    <method name=\"engine_add_link\" >\n"
	"      <arg direction=\"in\" type=\"as\" name=\"link\" />\n"
	"      <arg direction=\"in\" type=\"i\" name=\"category\" />\n"
	"    </method>\n"
	"  </interface>\n"
        "")

	public:
		EngineAdaptor(QObject *parent);
		virtual ~EngineAdaptor();

	public Q_SLOTS:
		void engine_add_link(const QString &link, const int &category);
};

#endif // PLASMAMULENGINE_H
