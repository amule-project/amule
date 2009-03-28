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

namespace amule.net {

    [SettingsGroupNameAttribute("XferControl")]
    public class amuleXferControlSettings : ApplicationSettingsBase {
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("true")]
        public bool FilenameVisible
        {
            get { return (bool)this["FilenameVisible"]; }
            set { this["FilenameVisible"] = value; }
        }
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("200")]
        public int FilenameWidth
        {
            get { return (int)this["FilenameWidth"]; }
            set { this["FilenameWidth"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("true")]
        public bool StatusVisible
        {
            get { return (bool)this["StatusVisible"]; }
            set { this["StatusVisible"] = value; }
        }
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("100")]
        public int StatusWidth
        {
            get { return (int)this["StatusWidth"]; }
            set { this["StatusWidth"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("true")]
        public bool SizeVisible
        {
            get { return (bool)this["SizeVisible"]; }
            set { this["StatusVisible"] = value; }
        }
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("100")]
        public int SizeWidth
        {
            get { return (int)this["SizeWidth"]; }
            set { this["StatusWidth"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("true")]
        public bool SpeedVisible
        {
            get { return (bool)this["SpeedVisible"]; }
            set { this["SpeedVisible"] = value; }
        }
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("100")]
        public int SpeedWidth
        {
            get { return (int)this["SpeedWidth"]; }
            set { this["SpeedWidth"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("true")]
        public bool CompletedVisible
        {
            get { return (bool)this["CompletedVisible"]; }
            set { this["CompletedVisible"] = value; }
        }
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("100")]
        public int CompletedWidth
        {
            get { return (int)this["CompletedWidth"]; }
            set { this["CompletedWidth"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("true")]
        public bool SourcesVisible
        {
            get { return (bool)this["SourcesVisible"]; }
            set { this["SourcesVisible"] = value; }
        }
        [UserScopedSetting()]
        [DefaultSettingValueAttribute("100")]
        public int SourcesWidth
        {
            get { return (int)this["SourcesWidth"]; }
            set { this["SourcesWidth"] = value; }
        }
    }

    public delegate void DownloadStatusListEventHandler();

    public class amuleDownloadStatusList : amuleListView, IContainerUI {
        DownloadQueueContainer m_item_container;
        amuleXferControlSettings m_settings = new amuleXferControlSettings();

        enum DOWNLOAD_CTRL_COL_ID {
            COL_FILENAME_ID = 0,
            COL_STATUS_ID,
            COL_SIZE_ID,
            COL_COMPLETED_ID,
            COL_SPEED_ID,
            COL_SOURCES_ID,

            COL_LAST_ID,
        };

        private ContextMenuStrip m_ctx_menu = new ContextMenuStrip();

        public event DownloadStatusListEventHandler OnCancelItem, OnPauseItem, OnResumeItem;

        void UpdateColumnIndexes()
        {
            int i = 0;
            m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_FILENAME_ID] = i;
            if ( m_settings.FilenameVisible ) i++;
            m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID] = i;
            if ( m_settings.StatusVisible ) i++;
            m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_SIZE_ID] = i;
            if ( m_settings.SizeVisible ) i++;
            m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_COMPLETED_ID] = i;
            if ( m_settings.CompletedVisible ) i++;
            m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_SPEED_ID] = i;
            if ( m_settings.SpeedVisible ) i++;
            m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_SOURCES_ID] = i;
            if ( m_settings.SourcesVisible ) i++;
        }

        void SaveSettigs()
        {
            if ( m_settings.FilenameVisible ) {
                m_settings.FilenameWidth =
                    Columns[m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_FILENAME_ID]].Width;
            }
            if ( m_settings.StatusVisible ) {
                m_settings.StatusWidth =
                    Columns[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID].Width;
            }
            m_settings.Save();
        }

        public amuleDownloadStatusList()
        {
            OwnerDraw = true;

            DrawColumnHeader +=
                new DrawListViewColumnHeaderEventHandler(amuleDownloadStatusList_DrawColumnHeader);

            DrawSubItem += new DrawListViewSubItemEventHandler(amuleDownloadStatusList_DrawSubItem);
            DownloadQueueItem.InitDraw3DModifiers(FontHeight + 1);

            MouseClick += new MouseEventHandler(amuleDownloadStatusList_MouseClickHandler);
            ColumnClick += new ColumnClickEventHandler(amuleDownloadStatusList_ColumtClickHandler);

            m_ctx_menu.Opening += new System.ComponentModel.CancelEventHandler(cms_Opening);
            m_ctx_menu.Items.Add(new ToolStripLabel("Downloads"));
            m_ctx_menu.Items.Add(new ToolStripSeparator());

            ToolStripButton it_pause = new ToolStripButton("Pause");
            it_pause.Click += new EventHandler(it_pause_Click);
            m_ctx_menu.Items.Add(it_pause);
            ContextMenuStrip = m_ctx_menu;

            ToolStripButton it_resume = new ToolStripButton("Resume");
            it_resume.Click += new EventHandler(it_resume_Click);
            m_ctx_menu.Items.Add(it_resume);

            ToolStripButton it_cancel = new ToolStripButton("Cancel");
            it_cancel.Click += new EventHandler(it_cancel_Click);
            m_ctx_menu.Items.Add(it_cancel);

            m_ctx_menu.Items.Add(new ToolStripSeparator());
            //
            // Init columns
            //
            m_column_index = new int[(int)DOWNLOAD_CTRL_COL_ID.COL_LAST_ID];
            UpdateColumnIndexes();
            // File name
            if ( m_settings.FilenameVisible ) {
                CreateColumtAt("File name", m_settings.FilenameWidth,
                    (int)DOWNLOAD_CTRL_COL_ID.COL_FILENAME_ID);
            }
            AppendItemToCtxMenu(m_ctx_menu, "File name", DOWNLOAD_CTRL_COL_ID.COL_FILENAME_ID,
                m_settings.FilenameVisible, new EventHandler(column_Click));
            // Status
            if ( m_settings.StatusVisible ) {
                CreateColumtAt("Status", m_settings.StatusWidth,
                    (int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID);
            }
            AppendItemToCtxMenu(m_ctx_menu, "Status", DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID,
                m_settings.StatusVisible, new EventHandler(column_Click));
            // Size
            if ( m_settings.SizeVisible ) {
                CreateColumtAt("Size", m_settings.SizeWidth,
                    (int)DOWNLOAD_CTRL_COL_ID.COL_SIZE_ID);
            }
            AppendItemToCtxMenu(m_ctx_menu, "Size", DOWNLOAD_CTRL_COL_ID.COL_SIZE_ID,
                m_settings.SizeVisible, new EventHandler(column_Click));
            // Completed size
            if ( m_settings.CompletedVisible ) {
                CreateColumtAt("Completed", m_settings.CompletedWidth,
                    (int)DOWNLOAD_CTRL_COL_ID.COL_COMPLETED_ID);
            }
            AppendItemToCtxMenu(m_ctx_menu, "Completed", DOWNLOAD_CTRL_COL_ID.COL_COMPLETED_ID,
                m_settings.CompletedVisible, new EventHandler(column_Click));
            // Speed
            if ( m_settings.SpeedVisible ) {
                CreateColumtAt("Speed", m_settings.SpeedWidth,
                    (int)DOWNLOAD_CTRL_COL_ID.COL_SPEED_ID);
            }
            AppendItemToCtxMenu(m_ctx_menu, "Speed", DOWNLOAD_CTRL_COL_ID.COL_SPEED_ID,
                m_settings.SpeedVisible, new EventHandler(column_Click));
            // Sources
            if ( m_settings.SourcesVisible ) {
                CreateColumtAt("Sources", m_settings.SourcesWidth,
                    m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_SOURCES_ID]);
            }
            AppendItemToCtxMenu(m_ctx_menu, "Sources", DOWNLOAD_CTRL_COL_ID.COL_SOURCES_ID,
                m_settings.SizeVisible, new EventHandler(column_Click));

            ContextMenuStrip = m_ctx_menu;
        }

        //
        // Click on column visibility checkbox in context menu
        //
        void column_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem col = sender as ToolStripMenuItem;
            DOWNLOAD_CTRL_COL_ID col_id = (DOWNLOAD_CTRL_COL_ID)col.Tag;
            bool status = !col.Checked;
            col.Checked = status;
            if ( !status ) {
                RemoveColumnAt((int)col_id);
            }
            switch ( col_id ) {
                case DOWNLOAD_CTRL_COL_ID.COL_FILENAME_ID:
                    m_settings.FilenameVisible = status;
                    UpdateColumnIndexes();
                    if ( status ) {
                        CreateColumtAt("File name", m_settings.FilenameWidth, (int)col_id);
                    }
                    break;
                case DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID:
                    m_settings.StatusVisible = status;
                    UpdateColumnIndexes();
                    if ( status ) {
                        CreateColumtAt("Status", m_settings.StatusWidth, (int)col_id);
                    }
                    break;
                case DOWNLOAD_CTRL_COL_ID.COL_SIZE_ID:
                    m_settings.SizeVisible = status;
                    UpdateColumnIndexes();
                    if ( status ) {
                        CreateColumtAt("Size", m_settings.SizeWidth, (int)col_id);
                    }
                    break;
                case DOWNLOAD_CTRL_COL_ID.COL_COMPLETED_ID:
                    m_settings.CompletedVisible = status;
                    UpdateColumnIndexes();
                    if ( status ) {
                        CreateColumtAt("Completed", m_settings.CompletedWidth, (int)col_id);
                    }
                    break;
                case DOWNLOAD_CTRL_COL_ID.COL_SPEED_ID:
                    m_settings.SpeedVisible = status;
                    UpdateColumnIndexes();
                    if ( status ) {
                        CreateColumtAt("Speed", m_settings.SpeedWidth, (int)col_id);
                    }
                    break;
                case DOWNLOAD_CTRL_COL_ID.COL_SOURCES_ID:
                    m_settings.SourcesVisible = status;
                    UpdateColumnIndexes();
                    if ( status ) {
                        CreateColumtAt("Sources", m_settings.SourcesWidth, (int)col_id);
                    }
                    break;
                default:
                    break;
            }
            Items.Clear();
            foreach ( DownloadQueueItem i in m_item_container.Items ) {
                DoInsertItem(i);
            }
        }

        //
        // "Cancel" command in context menu
        //
        void it_cancel_Click(object sender, EventArgs e)
        {
            OnCancelItem();
        }

        //
        // "Resume" command in context menu
        //
        void it_resume_Click(object sender, EventArgs e)
        {
            OnResumeItem();
        }

        //
        // "Pause" command in context menu
        //
        void it_pause_Click(object sender, EventArgs e)
        {
            OnPauseItem();
        }

        public void SelectedItemsToCommand(ecProto.ecPacket cmd)
        {
            foreach ( ListViewItem i in SelectedItems ) {
                DownloadQueueItem it = i.Tag as DownloadQueueItem;
                ecProto.ecTagMD5 tag = new ecProto.ecTagMD5(ECTagNames.EC_TAG_PARTFILE, it.ID);
                cmd.AddSubtag(tag);
            }
        }

        //
        // Context menu - on opening. Can support dynamic menu creation
        //
        void cms_Opening(object sender, System.ComponentModel.CancelEventArgs e)
        {
            
            e.Cancel = false;
        }

        protected override void OnHandleDestroyed(EventArgs e)
        {
            SaveSettigs();
            base.OnHandleDestroyed(e);
        }
        public DownloadQueueContainer ItemContainer
        {
            get { return m_item_container; }
            set
            {
                m_item_container = value;
                m_item_container.NewItemStatusLineLength = Columns[1].Width;
            }
        }

        override protected void OnColumnWidthChanged(ColumnWidthChangedEventArgs e)
        {
            int status_index = m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID];
            if ( (e.ColumnIndex == status_index) && m_settings.StatusVisible) {
                int new_size = Columns[status_index].Width + 1;
                m_item_container.NewItemStatusLineLength = new_size;

                foreach ( ListViewItem i in Items ) {
                    DownloadQueueItem it = i.Tag as DownloadQueueItem;
                    it.AllocColorLine(new_size);
                    it.DrawLine();
                }
            }
        }

        void amuleDownloadStatusList_MouseClickHandler(object o, MouseEventArgs e)
        {
            if ( e.Button == MouseButtons.Right ) {
                
            }
        }

        void amuleDownloadStatusList_ColumtClickHandler(object o, ColumnClickEventArgs e)
        {
        }

        unsafe void DrawStatusBar(DownloadQueueItem it, Graphics g, Rectangle posR)
        {
            //
            // Bitmap is created as 32bpp (rgb+alpha)
            //
            Bitmap status_bmp = new Bitmap(posR.Width, posR.Height,
                System.Drawing.Imaging.PixelFormat.Format32bppArgb);

            System.Drawing.Imaging.BitmapData bmd = status_bmp.LockBits(
                new Rectangle(0, 0, status_bmp.Width, status_bmp.Height),
                System.Drawing.Imaging.ImageLockMode.ReadWrite,
                System.Drawing.Imaging.PixelFormat.Format32bppArgb);

            RGB[] item_color_line = it.ColorLine;
            byte[] modifiers = DownloadQueueItem.Get_3D_Modifiers();

            for ( int y = 0; y < bmd.Height; y++ ) {
                byte* row = (byte*)bmd.Scan0 + (y * bmd.Stride);

                for ( int x = 0; x < bmd.Width; x++ ) {
                    UInt32* pixel_ptr = (UInt32*)(row + x * 4);
                    //row[x * 3 + 2] = 255;
                    //*pixel_ptr = 0xff0000; //RED
                    //*pixel_ptr = 0x00ff00; //GREEN
                    //*pixel_ptr = 0x1f0000ff; // BLUE
                    //*pixel_ptr = item_color_line[x] | (x << 24);
                    item_color_line[x].WriteToBuffWithModifier(pixel_ptr, modifiers[y]);
                }

            }

            status_bmp.UnlockBits(bmd);

            g.DrawImage(status_bmp, posR);
            status_bmp.Dispose();
        }

        void amuleDownloadStatusList_DrawSubItem(object sender, DrawListViewSubItemEventArgs e)
        {
            if ( (e.ColumnIndex == m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID])
                && m_settings.StatusVisible ) {
                // status is colored bar
                Rectangle r = e.Bounds;
                DownloadQueueItem it = e.Item.Tag as DownloadQueueItem;
                DrawStatusBar(it, e.Graphics, r);
                e.DrawDefault = false;
            }
            else {
                e.DrawBackground();
            }
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
            string first_col_txt;
            if ( m_settings.FilenameVisible ) {
                first_col_txt = i.Name;
            } else if ( m_settings.StatusVisible ) {
                first_col_txt = i.PercentDone;
            } else if ( m_settings.SizeVisible ) {
                first_col_txt = i.Size;
            } else if ( m_settings.CompletedVisible ) {
                first_col_txt = i.SizeDone;
            } else if ( m_settings.SpeedVisible ) {
                first_col_txt = i.Speed;
            } else if ( m_settings.SourcesVisible ) {
                first_col_txt = i.Sources;
            } else {
                first_col_txt = "all columns hidden";
            }

            ListViewItem it = new ListViewItem(first_col_txt);

            if ( m_settings.StatusVisible ) {
                it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.PercentDone));
            }
            if ( m_settings.SizeVisible ) {
                it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Size));
            }
            if ( m_settings.CompletedVisible ) {
                it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.SizeDone));
            }
            if ( m_settings.SpeedVisible ) {
                it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Speed));
            }
            if ( m_settings.SourcesVisible ) {
                it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Sources));
            }

            if ( m_settings.StatusVisible ) {
                it.SubItems[m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID]].ForeColor = Color.White;
                Columns[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID].TextAlign = HorizontalAlignment.Center;

            }
            it.Tag = i;

            Items.Add(it);

            i.UiItem = it;
        }

        void DoUpdateItem(DownloadQueueItem i)
        {
            ListViewItem it = i.UiItem as ListViewItem;
            int idx = m_column_index[(int)DOWNLOAD_CTRL_COL_ID.COL_STATUS_ID];
            if ( it.SubItems[idx].Text != i.PercentDone ) {
                it.SubItems[idx].Text = i.PercentDone;
            }
            idx = (int)DOWNLOAD_CTRL_COL_ID.COL_COMPLETED_ID;
            if ( it.SubItems[idx].Text != i.SizeDone ) {
                it.SubItems[idx].Text = i.SizeDone;
            }
            idx = (int)DOWNLOAD_CTRL_COL_ID.COL_SPEED_ID;
            if ( it.SubItems[idx].Text != i.Speed ) {
                it.SubItems[idx].Text = i.Speed;
            }
            //Items
        }

        #region IContainerUI Members

        delegate void UpdateCallback();

        public void MyEndUpdate()
        {
            if ( InvokeRequired ) {
                UpdateCallback d = new UpdateCallback(EndUpdate);
                Invoke(d);
            }
            else {
                EndUpdate();
            }
        }

        public void MyBeginUpdate()
        {
            if ( InvokeRequired ) {
                UpdateCallback d = new UpdateCallback(BeginUpdate);
                Invoke(d);
            }
            else {
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
            }
            else {
                DoInsertItem(it);
            }
        }

        public void UpdateItem(object i)
        {
            DownloadQueueItem it = i as DownloadQueueItem;
            if ( InvokeRequired ) {
                ItemCallback d = new ItemCallback(DoUpdateItem);
                Invoke(d, it);
            }
            else {
                DoUpdateItem(it);
            }
        }

        #endregion
    }

}
