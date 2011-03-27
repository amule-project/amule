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
#include "plasmamule-dbus.h"

#include <plasma/dataengine.h>
#include <plasma/applet.h>

#include <QApplication>
#include <QMenu>
#include <QStringList>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	Plasma::Applet* aMuleEngine = new Plasma::Applet;
	Plasma::DataEngine::Data data = aMuleEngine->dataEngine("plasmamule")->query(QString("cat_names"));

	QStringList cat_names = data["cat_names"].toStringList();
	QMenu *menu = new QMenu;
 
	for (QStringList::const_iterator constIterator = cat_names.constBegin(); constIterator != cat_names.constEnd(); constIterator++)
	{
		menu->addAction(*constIterator);
		if (constIterator != cat_names.constEnd())
		{
			menu->addSeparator();
		}
	}

	QAction *cat_selection = menu->exec(QCursor::pos());
	if (cat_selection)
	{
		for (int i = 1; i < argc; i++)
		{
			sendLinkToEngine (argv[i], cat_names.indexOf(cat_selection->text()), 0, 0);
		}
	}

	QCoreApplication::exit(0);
}
