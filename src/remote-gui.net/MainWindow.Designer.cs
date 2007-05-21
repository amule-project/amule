namespace amule.net
{
    partial class MainWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainWindow));
            this.panelStatus = new System.Windows.Forms.Panel();
            this.textLinktatus = new System.Windows.Forms.TextBox();
            this.panelToolbar = new System.Windows.Forms.Panel();
            this.buttonShared = new System.Windows.Forms.Button();
            this.buttonNetwork = new System.Windows.Forms.Button();
            this.buttonSearch = new System.Windows.Forms.Button();
            this.buttonXfer = new System.Windows.Forms.Button();
            this.panelMain = new System.Windows.Forms.Panel();
            this.panelStatus.SuspendLayout();
            this.panelToolbar.SuspendLayout();
            this.SuspendLayout();
            // 
            // panelStatus
            // 
            this.panelStatus.Controls.Add(this.textLinktatus);
            this.panelStatus.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panelStatus.Location = new System.Drawing.Point(0, 362);
            this.panelStatus.Name = "panelStatus";
            this.panelStatus.Size = new System.Drawing.Size(678, 24);
            this.panelStatus.TabIndex = 0;
            // 
            // textLinktatus
            // 
            this.textLinktatus.Dock = System.Windows.Forms.DockStyle.Left;
            this.textLinktatus.Location = new System.Drawing.Point(0, 0);
            this.textLinktatus.Name = "textLinktatus";
            this.textLinktatus.ReadOnly = true;
            this.textLinktatus.Size = new System.Drawing.Size(160, 20);
            this.textLinktatus.TabIndex = 0;
            // 
            // panelToolbar
            // 
            this.panelToolbar.Controls.Add(this.buttonShared);
            this.panelToolbar.Controls.Add(this.buttonNetwork);
            this.panelToolbar.Controls.Add(this.buttonSearch);
            this.panelToolbar.Controls.Add(this.buttonXfer);
            this.panelToolbar.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelToolbar.Location = new System.Drawing.Point(0, 0);
            this.panelToolbar.Name = "panelToolbar";
            this.panelToolbar.Size = new System.Drawing.Size(678, 45);
            this.panelToolbar.TabIndex = 1;
            // 
            // buttonShared
            // 
            this.buttonShared.Dock = System.Windows.Forms.DockStyle.Left;
            this.buttonShared.Image = global::remote_gui.net.Properties.Resources.Shared_Files;
            this.buttonShared.Location = new System.Drawing.Point(135, 0);
            this.buttonShared.Name = "buttonShared";
            this.buttonShared.Size = new System.Drawing.Size(45, 45);
            this.buttonShared.TabIndex = 3;
            this.buttonShared.UseVisualStyleBackColor = true;
            this.buttonShared.Click += new System.EventHandler(this.buttonShared_Click);
            // 
            // buttonNetwork
            // 
            this.buttonNetwork.Dock = System.Windows.Forms.DockStyle.Left;
            this.buttonNetwork.Image = ((System.Drawing.Image)(resources.GetObject("buttonNetwork.Image")));
            this.buttonNetwork.Location = new System.Drawing.Point(90, 0);
            this.buttonNetwork.Name = "buttonNetwork";
            this.buttonNetwork.Size = new System.Drawing.Size(45, 45);
            this.buttonNetwork.TabIndex = 2;
            this.buttonNetwork.UseVisualStyleBackColor = true;
            this.buttonNetwork.Click += new System.EventHandler(this.buttonNetwork_Click);
            // 
            // buttonSearch
            // 
            this.buttonSearch.Dock = System.Windows.Forms.DockStyle.Left;
            this.buttonSearch.Image = ((System.Drawing.Image)(resources.GetObject("buttonSearch.Image")));
            this.buttonSearch.Location = new System.Drawing.Point(45, 0);
            this.buttonSearch.Name = "buttonSearch";
            this.buttonSearch.Size = new System.Drawing.Size(45, 45);
            this.buttonSearch.TabIndex = 1;
            this.buttonSearch.UseVisualStyleBackColor = true;
            this.buttonSearch.Click += new System.EventHandler(this.buttonSearch_Click);
            // 
            // buttonXfer
            // 
            this.buttonXfer.Dock = System.Windows.Forms.DockStyle.Left;
            this.buttonXfer.Image = ((System.Drawing.Image)(resources.GetObject("buttonXfer.Image")));
            this.buttonXfer.Location = new System.Drawing.Point(0, 0);
            this.buttonXfer.Name = "buttonXfer";
            this.buttonXfer.Size = new System.Drawing.Size(45, 45);
            this.buttonXfer.TabIndex = 0;
            this.buttonXfer.UseVisualStyleBackColor = true;
            this.buttonXfer.Click += new System.EventHandler(this.buttonXfer_Click);
            // 
            // panelMain
            // 
            this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelMain.Location = new System.Drawing.Point(0, 45);
            this.panelMain.Name = "panelMain";
            this.panelMain.Size = new System.Drawing.Size(678, 317);
            this.panelMain.TabIndex = 2;
            // 
            // MainWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(678, 386);
            this.Controls.Add(this.panelMain);
            this.Controls.Add(this.panelToolbar);
            this.Controls.Add(this.panelStatus);
            this.Name = "MainWindow";
            this.Text = "MainWindow";
            this.Load += new System.EventHandler(this.MainWindow_Load);
            this.panelStatus.ResumeLayout(false);
            this.panelStatus.PerformLayout();
            this.panelToolbar.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panelStatus;
        private System.Windows.Forms.Panel panelToolbar;
        private System.Windows.Forms.Panel panelMain;
        private System.Windows.Forms.Button buttonXfer;
        private System.Windows.Forms.Button buttonSearch;
        private System.Windows.Forms.TextBox textLinktatus;
        private System.Windows.Forms.Button buttonNetwork;
        private System.Windows.Forms.Button buttonShared;

    }
}