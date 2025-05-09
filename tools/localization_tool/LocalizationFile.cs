using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using static EqLocalizationTool.KeyValues;

namespace EqLocalizationTool
{
	class LocalizationFile
    {
		public bool DontSaveNotLocalized { get; set; }
		private string GameDir;
		private string CategoryName;
		
		public LocalizationFile(string gameDir, string category)
        {
            GameDir = gameDir;
            CategoryName = category;
		}

        private string GetLangFileName(string langName)
        {
            string fileName = GameDir + "\\resources\\text_" + langName + "\\" + CategoryName + ".txt";
            return fileName;
		}

        public List<LocalizationData> ReadData(string langName)
        {
			List<LocalizationData> data = new List<LocalizationData>();

            var fileName = GetLangFileName(langName);
			var fileData = File.ReadAllText(fileName, Encoding.UTF8).ToArray();

			LocalizationData currentData = null;

			KV_Tokenizer(fileName, fileData, (tk, sig, args) => {
		        switch (sig[0])
		        {
					case 't':   // text token
					{                        
						if (currentData == null)
						{
							currentData = new LocalizationData();
							currentData.ID = (string)args[0];
						}
						else
						{
							currentData.Localized = (string)args[0];
						}

						break;
					}
					case 'b': // break
					{				       
						if(currentData != null)
							data.Add(currentData);
						currentData = null;
						break;
					}
		        }

                return EKVTokenState.KV_PARSE_RESUME;
			});
				
			return data;
        }

        public void WriteData(string local, List<LocalizationData> data)
        {
            string fileName = GetLangFileName(local);
            string fileNameBak = fileName + ".bak";

            if (File.Exists(fileNameBak))
                File.Delete(fileNameBak);

            File.Move(fileName, fileNameBak);

            using (StreamWriter sw = new StreamWriter(fileName, false, Encoding.UTF8))
            {
                foreach (var d in data)
                {
                    if (DontSaveNotLocalized && d.English == d.Localized)
                        continue;

                    string loc = d.Localized
						.Replace("\"", "\\\"")
						.Replace(Environment.NewLine, "\\n")
						.Replace("\t", "\\t")
						.Replace("\r", "\\r");

					sw.WriteLine($"{d.ID}\t\t\"{loc}\";");
                }
                sw.Close();
            }
        }
    }
}
