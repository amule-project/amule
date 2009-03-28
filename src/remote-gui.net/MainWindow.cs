//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2009 lfroen ( lfroen@gmail.com / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Configuration;

namespace amule.net
{
    public partial class MainWindow : Form
    {
        amuleRemote m_amuleRemote = new amuleRemote();
        Timer m_updateTimer = null;

        DownloadQueueContainer m_dload_info;
        SharedFileListContainer m_shared_info;

        amuleDownloadStatusList m_download_status_ctrl;
        amuleSharedFilesList m_shared_list_ctrl;

        amuleSearchControl m_search_ctrl;
        amuleSettings m_settings = new amuleSettings();

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
                        MessageBox.Show(errorMsg, "Connection to aMule core failed",
                            MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    }
                } else {
                    Application.Exit();
                    return;
                }
            }

            textLinktatus.Text = "aMule core on [" + amuleHost + ":" + amulePort + "]";
            Text = "aMule remote control [" + amuleHost + ":" + amulePort + "]";
            Size = m_settings.MainWindowSize;

            m_amuleRemote.SetECHandler(new amuleMainECHanler(this));

            //
            // Connection OK at this point
            //
            m_download_status_ctrl = new amuleDownloadStatusList();
            m_dload_info = new DownloadQueueContainer(m_download_status_ctrl);
            m_download_status_ctrl.ItemContainer = m_dload_info;

            m_download_status_ctrl.OnCancelItem +=
                new DownloadStatusListEventHandler(m_download_status_ctrl_OnCancelItem);
            m_download_status_ctrl.OnPauseItem +=
                new DownloadStatusListEventHandler(m_download_status_ctrl_OnPauseItem);
            m_download_status_ctrl.OnResumeItem +=
                new DownloadStatusListEventHandler(m_download_status_ctrl_OnResumeItem);

            m_shared_list_ctrl = new amuleSharedFilesList();
            m_shared_info = new SharedFileListContainer(m_shared_list_ctrl);

            m_search_ctrl = new amuleSearchControl();

            m_updateTimer = new Timer();
            m_updateTimer.Tag = this;
            m_updateTimer.Tick += new EventHandler(UpdateTimerProc);
            m_updateTimer.Interval = 1000;
            m_updateTimer.Start();

            // default - download list view
            panelMain.Controls.Add(m_download_status_ctrl);

            // for testing set needed state!
            m_req_state = UpdateRequestState.MainInfo;
        }

        void m_download_status_ctrl_OnResumeItem()
        {
            ecProto.ecPacket cmd = new ecProto.ecPacket(ECOpCodes.EC_OP_PARTFILE_RESUME);
            m_download_status_ctrl.SelectedItemsToCommand(cmd);
            m_amuleRemote.SendPacket(cmd);
        }

        void m_download_status_ctrl_OnPauseItem()
        {
            ecProto.ecPacket cmd = new ecProto.ecPacket(ECOpCodes.EC_OP_PARTFILE_PAUSE);
            m_download_status_ctrl.SelectedItemsToCommand(cmd);
            m_amuleRemote.SendPacket(cmd);
        }

        void m_download_status_ctrl_OnCancelItem()
        {
            string msg = "Following files will be cancelled:\n";
            foreach ( ListViewItem i in m_download_status_ctrl.SelectedItems ) {
                msg += i.Text + "\n";
            }
            if ( MessageBox.Show(msg, "Cancel files", MessageBoxButtons.OKCancel,
                MessageBoxIcon.Question) == DialogResult.OK ) {
                ecProto.ecPacket cmd = new ecProto.ecPacket(ECOpCodes.EC_OP_PARTFILE_DELETE);
                m_download_status_ctrl.SelectedItemsToCommand(cmd);
                m_amuleRemote.SendPacket(cmd);
            }
        }

        //
        // Process reply for "stats"
        //
        delegate void StatsReplyCallback(ecProto.ecPacket packet);
        public void StatsReplyInvoke(ecProto.ecPacket packet)
        {
            if ( InvokeRequired ) {
                StatsReplyCallback d = new StatsReplyCallback(StatsReply);
                Invoke(d, packet);
            } else {
                StatsReply(packet);
            }
        }

        void StatsReply(ecProto.ecPacket packet)
        {
            ecProto.ecTag t = null;
            t = packet.SubTag(ECTagNames.EC_TAG_STATS_DL_SPEED);
            int dl_speed = ((ecProto.ecTagInt)t).ValueInt();

            t = packet.SubTag(ECTagNames.EC_TAG_STATS_UL_SPEED);
            int ul_speed = ((ecProto.ecTagInt)t).ValueInt();
            //string server = ((ecProto.ecTagString)t).ToString();

            ecProto.ecConnStateTag connState =
                new ecProto.ecConnStateTag((ecProto.ecTagInt)packet.SubTag(ECTagNames.EC_TAG_CONNSTATE));

            textBoxDownSpeed.Text = "Down:" + ValueToPrefix(dl_speed) + "/s";
            textBoxUpSpeed.Text = "Up:" + ValueToPrefix(ul_speed) + "/s";
            if ( connState.IsConnected() ) {
                if (connState.IsConnectedED2K()) {
                    textLinktatus.Text = "ED2K: connected";
                    //ecProto.ecTagString server_name = (ecProto.ecTagString)server.SubTag(ECTagNames.EC_TAG_SERVER_NAME);
                    //toolStripStatusServer.Text = server_name.StringValue();
                } else {
                    //toolStripStatusServer.Text = "";
                    if (connState.IsConnectingED2K() ) {
                        textLinktatus.Text = "ED2K: connecting ...";
                    } else {
                        textLinktatus.Text = "ED2K: disconnected";
                    }
                }
                if (connState.IsConnectedKademlia()) {
                    //textLinktatus.Text = "KAD: connected";
                }
                
            }
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
            if ( panelMain.Controls[0] != m_search_ctrl ) {
                panelMain.Controls.Clear();
                panelMain.Controls.Add(m_search_ctrl);
            }
        }

        private void buttonShared_Click(object sender, EventArgs e)
        {
            if ( panelMain.Controls[0] != m_shared_list_ctrl ) {
                panelMain.Controls.Clear();
                panelMain.Controls.Add(m_shared_list_ctrl);
            }

        }

        private void buttonAddLink_Click(object sender, EventArgs e)
        {
            AddLinkDialog dlg = new AddLinkDialog();
            if ( dlg.ShowDialog() == DialogResult.OK ) {
                string link = dlg.textBoxLink.Text;
                ecProto.ecPacket cmd = new ecProto.ecPacket(ECOpCodes.EC_OP_ADD_LINK);
                ecProto.ecTagString linktag = new ecProto.ecTagString(ECTagNames.EC_TAG_STRING, link);
                cmd.AddSubtag(linktag);
                m_amuleRemote.SendPacket(cmd);
            }
        }

        private void buttonPrefs_Click(object sender, EventArgs e)
        {

        }

        private void buttonAbout_Click(object sender, EventArgs e)
        {
            AboutBox dlg = new AboutBox();
            dlg.ShowDialog();
        }

        private void MainWindow_FormClosed(object sender, FormClosedEventArgs e)
        {
            m_settings.MainWindowSize = Size;

            m_settings.Save();
        }

        public string ValueToPrefix(Int64 value)
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

    }

    [SettingsGroupNameAttribute("Application")]
    public class amuleSettings : ApplicationSettingsBase {

        [UserScopedSettingAttribute()]
        public string Host
        {
            get { return (string)this["Host"]; }
            set { this["Host"] = value; }
        }

        [UserScopedSettingAttribute()]
        public string Password
        {
            get { return (string)this["Password"]; }
            set { this["Password"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("500, 400")]
        public Size MainWindowSize
        {
            get { return (Size)this["MainWindowSize"]; }
            set { this["MainWindowSize"] = value; }
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
                    m_owner.StatsReplyInvoke(packet);
                    break;
                case ECOpCodes.EC_OP_DLOAD_QUEUE:
                    m_owner.DloadQueueReply(packet);
                    break;
                case ECOpCodes.EC_OP_SHARED_FILES:
                    m_owner.SharedFilesReply(packet);
                    break;
                case ECOpCodes.EC_OP_NOOP:
                    break;
                default:
                    throw new Exception("Unhandled EC reply");
            }
        }
    }

    public class amuleListView : ListView {
        protected int[] m_column_index;

        public amuleListView()
        {
            Dock = DockStyle.Fill;
            View = View.Details;
            DoubleBuffered = true;
            FullRowSelect = true;
            MultiSelect = true;
        }
        
        public void LoadColumns(string [] columns, int [] width)
        {
            int i = 0;
            foreach (string c in columns) {
                if ( width[i] == 0 ) {
                    continue;
                }
                ColumnHeader h = new ColumnHeader();
                h.Text = c;
                h.Width = width[i++];
                Columns.Add(h);
            }
        }

        public void CreateColumtAt(string name, int width, int index)
        {
            ColumnHeader h = new ColumnHeader();
            h.Text = name;
            h.Width = width;
            h.Tag = index;
            Columns.Insert(m_column_index[index], h);
        }
        public void RemoveColumnAt(int index)
        {
            Columns.RemoveAt(m_column_index[index]);
        }

        public void AppendItemToCtxMenu(ContextMenuStrip menu, string text,
            object tag, bool check_state, EventHandler target)
        {
            ToolStripMenuItem it = new ToolStripMenuItem(text);
            it.Click += target;
            it.CheckState = check_state ? CheckState.Checked : CheckState.Unchecked;
            it.Tag = tag;
            menu.Items.Add(it);
        }
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