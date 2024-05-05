using System;
using System.Windows.Forms;

namespace EqLocalizationTool
{
	public partial class FindTextDialog : Form
    {
        public FindTextDialog()
        {
            InitializeComponent();
        }

        private void btNext_Click(object sender, EventArgs e)
        {
			((MainForm)Owner).FindNext();            
        }

        private void btCancel_Click(object sender, EventArgs e)
        {
            Hide();
        }

        private void FindTextDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            if(e.CloseReason == CloseReason.UserClosing)
                e.Cancel = true;
            Hide();
        }

        private void rb_Click(object sender, EventArgs e)
        {
            ((RadioButton)sender).Checked = true;
        }

		private void FindTextDialog_Load(object sender, EventArgs e)
		{
            CenterToParent();
		}
	}
}
