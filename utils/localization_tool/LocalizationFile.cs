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
        private string gameDir;
		private string categoryName;
		public LocalizationFile(string gameDir, string category)
        {
            this.gameDir = gameDir;
            categoryName = category;
		}

        public bool DontSaveNotLocalized { get; set; }

        private string GetLangFileName(string langName)
        {
            string fileName = gameDir + "\\resources\\text_" + langName + "\\" + categoryName + ".txt";
            return fileName;
		}


        public List<LocalizationData> ReadData(string langName)
        {
			List<LocalizationData> data = new List<LocalizationData>();
            int l = 0;
            //try
            {
                var fileName = GetLangFileName(langName);

				LocalizationData currentData = null;

				KV_Tokenizer(fileName, File.ReadAllText(fileName, Encoding.UTF8).ToArray(), (tk, sig, args) => {
		            switch (sig[0])
		            {
			            case 'c':
			            {
				            // character filtering

				            // not supporting arrays on KV2
				            //if (*dataPtr == KV_ARRAY_SEPARATOR || *dataPtr == KV_ARRAY_BEGIN || *dataPtr == KV_ARRAY_END)
					        //    return KV_PARSE_ERROR;

				            break;
			            }
			            case 's':
			            {
				            // increase/decrease section depth
				            if (sig[1] == '+')
				            {
					            //if (currentSection == nullptr)
						        //    return KV_PARSE_ERROR;
                                //
					            //sectionStack.append(currentSection);
					            //currentSection = nullptr;
				            }
				            else if (sig[1] == '-')
				            {
					            //sectionStack.popBack();
					            //currentSection = nullptr;
				            }

				            break;
			            }
			            case 't':
			            {
                            // text token

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
			            case 'b':
			            {
				            // break
                            if(currentData != null)
                                data.Add(currentData);
				            currentData = null;
				            break;
			            }
		            }

                    return EKVTokenState.KV_PARSE_RESUME;
				});
				
			}
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
