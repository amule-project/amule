#ifndef PLASMAMULENGINE_H
#define PLASMAMULENGINE_H

#include <plasma/dataengine.h>
#include <QString>

class PlasmaMuleEngine : public Plasma::DataEngine
{
    // required since Plasma::DataEngine inherits QObject
    Q_OBJECT

	public:
		PlasmaMuleEngine(QObject* parent, const QVariantList& args);
		QStringList sources() const;
		void init ();

	protected:
		bool sourceRequestEvent(const QString& name);
		bool updateSourceEvent(const QString& name);

	private:
		bool m_OSActive;
		QString m_OSFile;
};

#endif // PLASMAMULENGINE_H
