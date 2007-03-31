namespace amule.net
{
    partial class ConnectDlg
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
            this.buttonConnect = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.amuleHost = new System.Windows.Forms.TextBox();
            this.amulePwd = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.amulePort = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // buttonConnect
            // 
            this.buttonConnect.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.buttonConnect.Location = new System.Drawing.Point(66, 190);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(128, 50);
            this.buttonConnect.TabIndex = 0;
            this.buttonConnect.Text = "&Connect";
            this.buttonConnect.UseVisualStyleBackColor = true;
            // 
            // buttonCancel
            // 
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(238, 190);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(128, 50);
            this.buttonCancel.TabIndex = 1;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(60, 46);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(59, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "aMule host";
            // 
            // amuleHost
            // 
            this.amuleHost.Location = new System.Drawing.Point(139, 43);
            this.amuleHost.Name = "amuleHost";
            this.amuleHost.Size = new System.Drawing.Size(112, 20);
            this.amuleHost.TabIndex = 3;
            this.amuleHost.Text = "leox";
            // 
            // amulePwd
            // 
            this.amulePwd.Location = new System.Drawing.Point(139, 106);
            this.amulePwd.Name = "amulePwd";
            this.amulePwd.PasswordChar = '*';
            this.amulePwd.Size = new System.Drawing.Size(112, 20);
            this.amulePwd.TabIndex = 5;
            this.amulePwd.Text = "123456";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(60, 109);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(53, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Password";
            // 
            // amulePort
            // 
            this.amulePort.Location = new System.Drawing.Point(296, 43);
            this.amulePort.MaxLength = 5;
            this.amulePort.Name = "amulePort";
            this.amulePort.Size = new System.Drawing.Size(66, 20);
            this.amulePort.TabIndex = 7;
            this.amulePort.Text = "9999";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(265, 46);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(25, 13);
            this.label3.TabIndex = 6;
            this.label3.Text = "port";
            // 
            // ConnectDlg
            // 
            this.AcceptButton = this.buttonConnect;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(432, 272);
            this.Controls.Add(this.amulePort);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.amulePwd);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.amuleHost);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonConnect);
            this.Name = "ConnectDlg";
            this.Text = "Connect";
            this.Load += new System.EventHandler(this.ConnectDlg_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button buttonConnect;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox amuleHost;
        private System.Windows.Forms.TextBox amulePwd;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox amulePort;
        private System.Windows.Forms.Label label3;

    }
}

