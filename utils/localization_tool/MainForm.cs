using System;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace EqLocalizationTool
{
	public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private void btnOpen_Click(object sender, EventArgs e)
        {
			var settings = Properties.Settings.Default;

			folderBrowserDialog1.SelectedPath = settings.InitialDir;

			if (folderBrowserDialog1.ShowDialog(this) != DialogResult.OK)
                return;

			tbGameDir.Text = folderBrowserDialog1.SelectedPath;

			string path = tbGameDir.Text;

            var allEnglishFiles = Directory
                .GetFiles(path + "\\resources\\text_english\\", "*.txt")
                .OrderBy(x => x)
                .Select(x => Path.GetFileNameWithoutExtension(x));

            cbCategory.Items.Clear();
			foreach (string fn in allEnglishFiles)
            {
                cbCategory.Items.Add(fn);
			}

            var allLangs = Directory
                .GetDirectories(path + "\\resources\\", "text_*")
                .OrderBy(x => x)
                .Select(x => Path.GetFileName(x).Remove(0, "text_".Length));

			cbLocal.Items.Clear();
			foreach (string fn in allLangs)
			{
                if(!fn.EndsWith("english"))
				    cbLocal.Items.Add(fn);
			}
            cbLocal.SelectedIndex = 0;

			localizationDataBindingSource.DataSource = new List<LocalizationData>();
            lblPerc.Text = "";
            if (settings.DefLang != "")
            {
                int i = cbLocal.Items.IndexOf(settings.DefLang);
                if (i >= 0)
                    cbLocal.SelectedIndex = i;
            }

            if(settings.DefCategory != "")
            {
				int i = cbCategory.Items.IndexOf(settings.DefCategory);
				if (i >= 0)
					cbCategory.SelectedIndex = i;
			}
		}

        private void btnAdd_Click(object sender, EventArgs e)
        {
            AddLocal local = new AddLocal();
            if (local.ShowDialog(this) == DialogResult.OK)
                cbLocal.Items.Add(local.tbLocal.Text);
        }

        LocalizationFile file;

        bool IsLocalizedToken(string en, string lc)
        {
            if (lc == null)
                return false;
            else if (en.Length <= 2)
                return true;
            else if (en.Length == lc.Length)
            {
                if (en != lc)
                    return true;
                else
                {
                    foreach (char c in en)
                        if (char.IsLetter(c))
                            return false;
                    // skip text without letters
                    return true;
                }
            }
            else
                return true;
        }

        private void OnLanguageOrCategoryChanged()
        {
            if (cbCategory.SelectedItem == null)
                return;

			file = new LocalizationFile(tbGameDir.Text, (string)cbCategory.SelectedItem);

			List<LocalizationData> eng = file.ReadData("english");
			List<LocalizationData> loc = file.ReadData((string)cbLocal.SelectedItem);

			foreach (var en in eng)
			{
				var lc = loc.SingleOrDefault(l => l.ID == en.ID);
				en.English = en.Localized;

				if (lc != null && lc.Localized != null)
					en.Localized = lc.Localized;
			}

			localizationDataBindingSource.DataSource = eng;
		}

		private void dataGridView1_DataBindingComplete(object sender, DataGridViewBindingCompleteEventArgs e)
		{
            if (!(localizationDataBindingSource.DataSource is List<LocalizationData>))
                return;

			List<LocalizationData> loc = (List<LocalizationData>)localizationDataBindingSource.DataSource;
            int tcount = 0;
			int lcount = 0;
            foreach (var lc in loc)
            {
				if (IsLocalizedToken(lc.English, lc.Localized))
				{
					lcount++;
				}
                else
                {
					dataGridView1.Rows[tcount].Cells[2].Style.BackColor = Color.Red;
				}
				tcount++;
			}

			if (tcount == 0)
				tcount = 1;
			lblPerc.Text = string.Format("{0:F}%", (1.0f * lcount / tcount) * 100);
		}

		private void cbCategory_SelectedIndexChanged(object sender, EventArgs e)
		{
            OnLanguageOrCategoryChanged();
		}

		private void cbLocal_SelectedIndexChanged(object sender, EventArgs e)
        {
			OnLanguageOrCategoryChanged();
		}

        private void btnSave_Click(object sender, EventArgs e)
        {
            if (file == null)
                return;
            List<LocalizationData> loc = (List<LocalizationData>)localizationDataBindingSource.DataSource;
            file.DontSaveNotLocalized = cbDontSaveNotLocalized.Checked;
            file.WriteData((string)cbLocal.SelectedItem, loc);
        }

        private void btnNext_Click(object sender, EventArgs e)
        {
            LocalizationData data;
            do 
            {
                localizationDataBindingSource.MoveNext();
                data = (LocalizationData) localizationDataBindingSource.Current;
            } while ((localizationDataBindingSource.Position + 1 < localizationDataBindingSource.Count) &&
                (data.ID == null || IsLocalizedToken(data.English, data.Localized)));
            Invalidate();
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            var settings = Properties.Settings.Default;

			settings.FindText = dialog.tbText.Text;
			settings.IDDefault = dialog.rbID.Checked;
			settings.EnglishDefault = dialog.rbEnglish.Checked;
			settings.LocalizedDefault = dialog.rbLocalized.Checked;

			if (folderBrowserDialog1.SelectedPath != "")
				settings.InitialDir = folderBrowserDialog1.SelectedPath;

			if (cbLocal.Text != "")
				settings.DefLang = cbLocal.Text;

			if (cbCategory.Text != "")
				settings.DefCategory = cbCategory.Text;

			settings.Save();
        }

		FindTextDialog dialog = new FindTextDialog();

		private void btFind_Click(object sender, EventArgs e)
		{
			dialog.Show(this);
		}

		internal void FindNext()
        {
            string searchText = dialog.tbText.Text.ToLower();
            int ind = dialog.rbID.Checked ? 0 : dialog.rbEnglish.Checked ? 1 : 2;
            if (searchText == "")
                return;

            bool gotoNext;
            do
            {
                localizationDataBindingSource.MoveNext();
				var data = (LocalizationData)localizationDataBindingSource.Current;
                gotoNext = (localizationDataBindingSource.Position + 1 < localizationDataBindingSource.Count);
                if (!gotoNext)
                    continue;

                string text = null;
				switch (ind)
                {
                    case 0:
						text = data.ID;
                        break;
                    case 1:
						text = data.English;
                        break;
                    case 2:
						text = data.Localized;
                        break;
                }
                if(text != null)
				    gotoNext = !text.ToLower().Contains(searchText);
			} while (gotoNext);

            Invalidate();
        }

        private void MainForm_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Control && e.KeyCode == Keys.Q)
                btnNext_Click(null, null);
            else if (e.Control && e.KeyCode == Keys.F)
                btFind_Click(null, null);
            else if (e.KeyCode == Keys.F3)
                FindNext();            
        }
	}
}
