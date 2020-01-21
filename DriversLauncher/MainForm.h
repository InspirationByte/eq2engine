#pragma once

namespace DriversLauncher {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Сводка для MainForm
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			//
			//TODO: добавьте код конструктора
			//
		}

	protected:
		/// <summary>
		/// Освободить все используемые ресурсы.
		/// </summary>
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::Button^  playBtn;
	private: System::Windows::Forms::ComboBox^  m_resolutionList;
	private: System::Windows::Forms::CheckBox^  m_fullscreen;
	private: System::Windows::Forms::ComboBox^  m_langSel;
	protected:

	private:
		/// <summary>
		/// Обязательная переменная конструктора.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Требуемый метод для поддержки конструктора — не изменяйте 
		/// содержимое этого метода с помощью редактора кода.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MainForm::typeid));
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->playBtn = (gcnew System::Windows::Forms::Button());
			this->m_resolutionList = (gcnew System::Windows::Forms::ComboBox());
			this->m_fullscreen = (gcnew System::Windows::Forms::CheckBox());
			this->m_langSel = (gcnew System::Windows::Forms::ComboBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// pictureBox1
			// 
			this->pictureBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->pictureBox1->BackColor = System::Drawing::Color::Black;
			this->pictureBox1->BackgroundImage = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pictureBox1.BackgroundImage")));
			this->pictureBox1->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
			this->pictureBox1->Location = System::Drawing::Point(12, 12);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(537, 217);
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			// 
			// playBtn
			// 
			this->playBtn->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->playBtn->Location = System::Drawing::Point(435, 235);
			this->playBtn->Name = L"playBtn";
			this->playBtn->Size = System::Drawing::Size(114, 28);
			this->playBtn->TabIndex = 1;
			this->playBtn->Text = L"Save && Play";
			this->playBtn->UseVisualStyleBackColor = true;
			this->playBtn->Click += gcnew System::EventHandler(this, &MainForm::playBtn_Click);
			// 
			// m_resolutionList
			// 
			this->m_resolutionList->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->m_resolutionList->FormattingEnabled = true;
			this->m_resolutionList->Location = System::Drawing::Point(12, 238);
			this->m_resolutionList->Name = L"m_resolutionList";
			this->m_resolutionList->Size = System::Drawing::Size(208, 21);
			this->m_resolutionList->TabIndex = 2;
			// 
			// m_fullscreen
			// 
			this->m_fullscreen->Anchor = System::Windows::Forms::AnchorStyles::Bottom;
			this->m_fullscreen->AutoSize = true;
			this->m_fullscreen->Location = System::Drawing::Point(226, 242);
			this->m_fullscreen->Name = L"m_fullscreen";
			this->m_fullscreen->Size = System::Drawing::Size(74, 17);
			this->m_fullscreen->TabIndex = 3;
			this->m_fullscreen->Text = L"Fullscreen";
			this->m_fullscreen->UseVisualStyleBackColor = true;
			// 
			// m_langSel
			// 
			this->m_langSel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->m_langSel->DisplayMember = L"Text";
			this->m_langSel->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->m_langSel->FormattingEnabled = true;
			this->m_langSel->Location = System::Drawing::Point(308, 238);
			this->m_langSel->Name = L"m_langSel";
			this->m_langSel->Size = System::Drawing::Size(121, 21);
			this->m_langSel->TabIndex = 4;
			this->m_langSel->ValueMember = L"Lang";
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(565, 275);
			this->Controls->Add(this->m_langSel);
			this->Controls->Add(this->m_fullscreen);
			this->Controls->Add(this->m_resolutionList);
			this->Controls->Add(this->playBtn);
			this->Controls->Add(this->pictureBox1);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedToolWindow;
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"MainForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"The Driver Syndicate - Configurator";
			this->Load += gcnew System::EventHandler(this, &MainForm::MainForm_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void MainForm_Load(System::Object^  sender, System::EventArgs^  e);
	private: System::Void playBtn_Click(System::Object^  sender, System::EventArgs^  e);
};
}
