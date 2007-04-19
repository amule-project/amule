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

        public MainWindow()
        {
            InitializeComponent();
        }

        enum UpdateRequestState { Stats, MainInfo };
        UpdateRequestState m_req_state;

        private static void UpdateTimerProc(Object myObject,
                                            EventArgs myEventArgs)
        {
            MainWindow w = (MainWindow)((Timer)myObject).Tag;

            //w.m_updateTimer.Stop();

            ecProto.ecPacket req = null;
            switch(w.m_req_state) {
                case UpdateRequestState.Stats:
                    req = new ecProto.ecPacket(ECOpCodes.EC_OP_STAT_REQ);
                    w.m_req_state = UpdateRequestState.MainInfo;
                    break;
                case UpdateRequestState.MainInfo:
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
            toolStripConnectionStatus.Text = "aMule core on [" + amuleHost + ":" + amulePort + "]";

            m_amuleRemote.SetECHandler(new amuleMainECHanler(this));

            //
            // Connection OK at this point
            //
            m_updateTimer = new Timer();
            m_updateTimer.Tag = this;
            m_updateTimer.Tick += new EventHandler(UpdateTimerProc);
            m_updateTimer.Interval = 1000;
            m_updateTimer.Start();
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

        }

        private void toolStripButtonDL_Click(object sender, EventArgs e)
        {
            ListView mv = new ListView();
            mv.Dock = DockStyle.Fill;
            
            toolStripContainerMain.ContentPanel.Controls.Add(mv);
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
            }
        }
    }


}