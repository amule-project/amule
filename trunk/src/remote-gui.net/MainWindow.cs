using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace amule.net
{
    public partial class MainWindow : Form
    {
        amuleRemote m_amuleRemote = new amuleRemote();
        Timer m_updateTimer = null;

        DownloadQueueContainer m_dload_info;


        amuleDownloadStatusList m_download_status_ctrl = new amuleDownloadStatusList();
        amuleSharedFilesList m_shared_list_ctrl = new amuleSharedFilesList();

        public MainWindow()
        {
            InitializeComponent();
        }

        enum UpdateRequestState { Stats, MainInfo };
        UpdateRequestState m_req_state;

        ecProto.ecPacket GetFullInfoRequest()
        {
            ecProto.ecPacket req = null;

            req = m_dload_info.ReQuery();

            return req;
        }

        private static void UpdateTimerProc(Object myObject,
                                            EventArgs myEventArgs)
        {
            MainWindow w = (MainWindow)((Timer)myObject).Tag;

            // TODO: for testing 1 request is enough
            w.m_updateTimer.Stop();

            ecProto.ecPacket req = null;
            switch(w.m_req_state) {
                case UpdateRequestState.Stats:
                    req = new ecProto.ecPacket(ECOpCodes.EC_OP_STAT_REQ);
                    w.m_req_state = UpdateRequestState.MainInfo;
                    break;
                case UpdateRequestState.MainInfo:
                    req = w.GetFullInfoRequest();
                    w.m_req_state = UpdateRequestState.Stats;
                    break;
            }
            w.m_amuleRemote.SendPacket(req);
        }

        string ValueToPrefix(Int64 value)
        {
            if ( value < 1024 ) {
                return string.Format("{0} bytes", value);
            } else if ( value < 1048576 ) {
                return string.Format("{0:f} Kb", ((float)value) / 1024);
            } else if ( value < 1073741824 ) {
                return string.Format("{0:f} Mb", ((float)value) / 1048576);
            } else {
                return string.Format("{0:f} Gb", ((float)value) / 1073741824);
            }
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            //
            // First, attempt to connect to remote core
            //
            ConnectDlg connectDlg = new ConnectDlg();
            string errorMsg = null;
            bool connect_ok = false;
            string amuleHost = "", amulePort = "";
            while ( !connect_ok ) {
                if (connectDlg.ShowDialog() == DialogResult.OK) {
                    amuleHost = connectDlg.Host();
                    amulePort = connectDlg.Port();
                    string pass = connectDlg.Pass();
                    int amulePort_int = 0;
                    try {
                        amulePort_int = Convert.ToInt16(amulePort, 10);
                    }catch(Exception) {
                        MessageBox.Show("Invalid port number", "Error",
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                        continue;
                    }
                    connect_ok = m_amuleRemote.ConnectToCore(amuleHost, amulePort_int, pass, ref errorMsg);
                    if (!connect_ok) {
                        Console.WriteLine("Connect failed '{0}'", errorMsg);
                    }
                } else {
                    Application.Exit();
                    return;
                }
            }

            // default - download list view
            panelMain.Controls.Add(m_download_status_ctrl);

            textLinktatus.Text = "aMule core on [" + amuleHost + ":" + amulePort + "]";

            m_amuleRemote.SetECHandler(new amuleMainECHanler(this));

            //
            // Connection OK at this point
            //
            m_dload_info = new DownloadQueueContainer(m_download_status_ctrl);

            m_updateTimer = new Timer();
            m_updateTimer.Tag = this;
            m_updateTimer.Tick += new EventHandler(UpdateTimerProc);
            m_updateTimer.Interval = 1000;
            m_updateTimer.Start();

            // for testing set needed state!
            m_req_state = UpdateRequestState.MainInfo;
        }

        //
        // Process reply for "stats"
        //
        public void StatsReply(ecProto.ecPacket packet)
        {
            ecProto.ecTag t = null;
            t = packet.SubTag(ECTagNames.EC_TAG_STATS_DL_SPEED);
            int dl_speed = ((ecProto.ecTagInt)t).ValueInt();

            t = packet.SubTag(ECTagNames.EC_TAG_STATS_UL_SPEED);
            int ul_speed = ((ecProto.ecTagInt)t).ValueInt();
            //string server = ((ecProto.ecTagString)t).ToString();

            ecProto.ecConnStateTag connState =
                new ecProto.ecConnStateTag((ecProto.ecTagInt)packet.SubTag(ECTagNames.EC_TAG_CONNSTATE));

            ecProto.ecTag server = connState.Server();
            /*
            toolStripXferDown.Text = ValueToPrefix(dl_speed) + "/s";
            toolStripXferUp.Text = ValueToPrefix(ul_speed) + "/s";
            if ( connState.IsConnected() ) {
                if (connState.IsConnectedED2K()) {
                    toolStripStatusED2K.Text = "ED2K: connected";
                    ecProto.ecTagString server_name = (ecProto.ecTagString)server.SubTag(ECTagNames.EC_TAG_SERVER_NAME);
                    toolStripStatusServer.Text = server_name.StringValue();
                } else {
                    toolStripStatusServer.Text = "";
                    if (connState.IsConnectingED2K() ) {
                        toolStripStatusED2K.Text = "ED2K: connecting ...";
                    } else {
                        toolStripStatusED2K.Text = "ED2K: disconnected";
                    }
                }
                if (connState.IsConnectedKademlia()) {
                    toolStripStatusKad.Text = "KAD: connected";
                }
                
            }
            */
        }

        public void DloadQueueReply(ecProto.ecPacket packet)
        {
            ecProto.ecPacket reply = m_dload_info.HandlePacket(packet);
            if ( reply != null ) {
                m_amuleRemote.SendPacket(reply);
            }
        }

        private void buttonXfer_Click(object sender, EventArgs e)
        {
            if ( panelMain.Controls[0] != m_download_status_ctrl ) {
                panelMain.Controls.Clear();
                panelMain.Controls.Add(m_download_status_ctrl);
            }
        }

        private void buttonNetwork_Click(object sender, EventArgs e)
        {
            UpdateTimerProc(m_updateTimer, null);
        }

        private void buttonSearch_Click(object sender, EventArgs e)
        {

        }

        private void buttonShared_Click(object sender, EventArgs e)
        {
            if ( panelMain.Controls[0] != m_shared_list_ctrl ) {
                panelMain.Controls.Clear();
                panelMain.Controls.Add(m_shared_list_ctrl);
            }

        }
    }

    public class amuleMainECHanler : amuleECHandler {
        MainWindow m_owner = null;
        public amuleMainECHanler(MainWindow o)
        {
            m_owner = o;
        }

        public override void HandlePacket(ecProto.ecPacket packet)
        {
            ECOpCodes op = packet.Opcode();
            switch(op) {
                case ECOpCodes.EC_OP_STATS:
                    m_owner.StatsReply(packet);
                    break;
                case ECOpCodes.EC_OP_DLOAD_QUEUE:
                    m_owner.DloadQueueReply(packet);
                    break;
            }
        }
    }

    public class amuleListView : ListView {
        public amuleListView()
        {
            Dock = DockStyle.Fill;
            View = View.Details;
        }

        public void LoadColumns(string [] columns, int [] width)
        {
            int i = 0;
            foreach (string c in columns) {
                ColumnHeader h = new ColumnHeader();
                h.Text = c;
                h.Width = width[i++];
                Columns.Add(h);
            }
        }
    }

    public class amuleDownloadStatusList : amuleListView {
        public amuleDownloadStatusList()
        {
            string[] columns = { "File name", "Status", "Size", "Transferred" };
            int[] width = { 200, 100, 100, 100 };
            LoadColumns(columns, width);

            OwnerDraw = true;
            DrawColumnHeader +=
                new DrawListViewColumnHeaderEventHandler(amuleDownloadStatusList_DrawColumnHeader);

            DrawSubItem += new DrawListViewSubItemEventHandler(amuleDownloadStatusList_DrawSubItem);
        }

        void amuleDownloadStatusList_DrawSubItem(object sender, DrawListViewSubItemEventArgs e)
        {
            e.DrawBackground();
            e.DrawText();
        }

        void amuleDownloadStatusList_DrawColumnHeader(object sender, DrawListViewColumnHeaderEventArgs e)
        {
            e.DrawBackground();
            e.DrawText();
        }

        ///
        // interface to core
        ///
        public void AddItem(DownloadQueueItem i)
        {
            ListViewItem it = new ListViewItem(i.Name);
            if ( InvokeRequired ) {
                AddItemCallback d = new AddItemCallback(AddItem);
                Invoke(d, i);
            } else {
                Items.Add(it);
            }
        }
        delegate void AddItemCallback(DownloadQueueItem i);

    }
    public class amuleSharedFilesList : amuleListView {
        public amuleSharedFilesList()
        {
            string[] columns = { "File name", "Size" };
            int[] width = { 100, 100, 100, 100 };
            LoadColumns(columns, width);
        }
    }
}