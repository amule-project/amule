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
        SharedFileListContainer m_shared_info;

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
            Control current_ctrl = panelMain.Controls[0];
            if (current_ctrl == m_download_status_ctrl) {
                req = m_dload_info.ReQuery();
            } else if (current_ctrl == m_shared_list_ctrl) {
                req = m_shared_info.ReQuery();
                m_updateTimer.Stop();
                if (req == null) {
                    throw new Exception("m_shared_info.ReQuery()");
                }
            }

            if ( req == null ) {
                throw new Exception("unhandled GUI state");
            }
            return req;
        }

        private static void UpdateTimerProc(Object myObject,
                                            EventArgs myEventArgs)
        {
            MainWindow w = (MainWindow)((Timer)myObject).Tag;

            // TODO: for testing 1 request is enough
            //w.m_updateTimer.Stop();

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
            m_shared_info = new SharedFileListContainer(m_shared_list_ctrl);

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

        public void SharedFilesReply(ecProto.ecPacket packet)
        {
            ecProto.ecPacket reply = m_shared_info.HandlePacket(packet);
            if (reply != null) {
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
                case ECOpCodes.EC_OP_SHARED_FILES:
                    m_owner.SharedFilesReply(packet);
                    break;
                default:
                    throw new Exception("Unhandled EC reply");
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

    public class amuleDownloadStatusList : amuleListView, IContainerUI {
        public amuleDownloadStatusList()
        {
            string[] columns = { "File name", "Status", "Size", "Completed", "Speed"};
            int[] width = { 300, 100, 100, 100, 100 };
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
            if ( e.Item.Selected ) {
                e.DrawFocusRectangle(e.Bounds);
            }
        }

        void amuleDownloadStatusList_DrawColumnHeader(object sender, DrawListViewColumnHeaderEventArgs e)
        {
            e.DrawBackground();
            e.DrawText();
        }

        ///
        // interface to core
        ///
        void DoInsertItem(DownloadQueueItem i)
        {
            ListViewItem it = new ListViewItem(i.Name);
            
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.PercentDone));
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Size));
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.SizeDone));
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Speed));
            Items.Add(it);

            i.UiItem = it;
        }

        void DoUpdateItem(DownloadQueueItem i)
        {
            ListViewItem it =  i.UiItem as ListViewItem;
            it.SubItems[1].Text = i.PercentDone;
            it.SubItems[3].Text = i.SizeDone;
            it.SubItems[4].Text = i.Speed;
            //Items
        }

        #region IContainerUI Members

        delegate void UpdateCallback();

        public void MyEndUpdate()
        {
            if (InvokeRequired) {
                UpdateCallback d = new UpdateCallback(EndUpdate);
                Invoke(d);
            } else {
                EndUpdate();
            }            
        }

        public void MyBeginUpdate()
        {
            if (InvokeRequired) {
                UpdateCallback d = new UpdateCallback(BeginUpdate);
                Invoke(d);
            } else {
                BeginUpdate();
            }
        }

        delegate void ItemCallback(DownloadQueueItem i);

        public void InsertItem(object i)
        {
            DownloadQueueItem it = i as DownloadQueueItem;
            if ( InvokeRequired ) {
                ItemCallback d = new ItemCallback(DoInsertItem);
                Invoke(d, it);
            } else {
                DoInsertItem(it);
            }
        }

        public void UpdateItem(object i)
        {
            DownloadQueueItem it = i as DownloadQueueItem;
            if (InvokeRequired) {
                ItemCallback d = new ItemCallback(DoUpdateItem);
                Invoke(d, it);
            } else {
                DoUpdateItem(it);
            }
        }

        #endregion
    }


    public class amuleSharedFilesList : amuleListView, IContainerUI {
        public amuleSharedFilesList()
        {
            string[] columns = { "File name", "Size" };
            int[] width = { 300, 100, 100, 100 };
            LoadColumns(columns, width);
        }

        #region IContainerUI Members

        delegate void UpdateCallback();
        void IContainerUI.MyEndUpdate()
        {
            if (InvokeRequired) {
                UpdateCallback d = new UpdateCallback(EndUpdate);
                Invoke(d);
            } else {
                EndUpdate();
            }
        }

        void IContainerUI.MyBeginUpdate()
        {
            if (InvokeRequired) {
                UpdateCallback d = new UpdateCallback(BeginUpdate);
                Invoke(d);
            } else {
                BeginUpdate();
            }
        }

        delegate void ItemCallback(SharedFileItem i);

        void IContainerUI.InsertItem(object i)
        {
            SharedFileItem it = i as SharedFileItem;
            if (InvokeRequired) {
                ItemCallback d = new ItemCallback(DoInsertItem);
                Invoke(d, it);
            } else {
                DoInsertItem(it);
            }
        }

        void IContainerUI.UpdateItem(object i)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        void DoInsertItem(SharedFileItem i)
        {
            ListViewItem it = new ListViewItem(i.Name);

            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Size));

            Items.Add(it);

            i.UiItem = it;
        }

        void DoUpdateItem(SharedFileItem i)
        {
            ListViewItem it = i.UiItem as ListViewItem;
            //Items
        }

        #endregion
    }
}