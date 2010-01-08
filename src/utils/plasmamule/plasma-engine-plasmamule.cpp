#include "plasma-engine-plasmamule.h"

#include <plasma/datacontainer.h>
#include <QDir>
#include <kdebug.h>
#include <QFile>

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
		<< "update_interval"
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
	QString Home = Dir.homePath();
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
		} else if (line.contains ("OnlineSignatureUpdate"))
		{
			setPollingInterval (line.remove (0,line.indexOf ("=")+1).toInt() * 1000);
			setData(I18N_NOOP ("update_interval"), line.remove (0,line.indexOf ("=")+1).toInt() * 1000);
		}
        }
	config_file.close ();
	setName("plasmamule");
	setData(I18N_NOOP ("config_found"), TRUE);
}

bool PlasmaMuleEngine::updateSourceEvent(const QString &name)
{
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
		return true;
	} else {
		return false;
	}
}

K_EXPORT_PLASMA_DATAENGINE(plasmamule, PlasmaMuleEngine)

#include "plasma-engine-plasmamule.moc"
