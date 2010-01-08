#ifndef plasmamule_applet_h
#define plasmamule_applet_h

#include <plasma/applet.h>
#include <plasma/dataengine.h>

class PlasmaMuleApplet : public Plasma::Applet
{
	Q_OBJECT
	public:
		PlasmaMuleApplet(QObject *parent, const QVariantList &args);
		~PlasmaMuleApplet();

		void paintInterface(QPainter *painter,
			const QStyleOptionGraphicsItem *option,
			const QRect& contentsRect);
		void init();
		
	public Q_SLOTS:
		void onSourceAdded(const QString&);
		void onSourceRemoved(const QString&);
		void dataUpdated(const QString&, const Plasma::DataEngine::Data&);

	private:
		void connectToEngine();
		QString calcSize (qlonglong);

		int m_update_interval, m_ed2k_state, m_ed2k_server_port, m_kad_status, m_clients_in_up_queue, m_shared_files_count, m_uptime;
		QString  m_ed2k_id_high_low, m_ed2k_server_name, m_ed2k_server_ip, m_nickname, m_version;
		double m_down_speed, m_up_speed;
		qlonglong m_total_bytes_downloaded, m_total_bytes_uploaded, m_session_bytes_downloaded, m_session_bytes_uploaded;
		bool m_os_active, m_config_found;
		Plasma::Svg m_svg;
		Plasma::DataEngine* m_aMuleEngine;
};
 
#endif
