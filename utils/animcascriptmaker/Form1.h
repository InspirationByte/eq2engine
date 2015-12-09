#pragma once

namespace animcascriptmaker {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// —водка дл€ Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: добавьте код конструктора
			//
		}

	protected:
		/// <summary>
		/// ќсвободить все используемые ресурсы.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::MenuStrip^  menuStrip1;
	protected: 
	private: System::Windows::Forms::ToolStripMenuItem^  fileToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  newToolStripMenuItem;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripMenuItem1;
	private: System::Windows::Forms::ToolStripMenuItem^  openToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  saveToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  saveAsToolStripMenuItem;

	private: System::Windows::Forms::ToolStripMenuItem^  editToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  editToolStripMenuItem1;

	private: System::Windows::Forms::ToolStripMenuItem^  sequenceToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  helpToolStripMenuItem;
	private: System::Windows::Forms::ToolStripSeparator^  exitToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  exitToolStripMenuItem1;
	private: System::Windows::Forms::ToolStripSeparator^  removeToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  removeToolStripMenuItem1;
	private: System::Windows::Forms::ToolStripMenuItem^  editToolStripMenuItem2;
	private: System::Windows::Forms::ToolStripSeparator^  removeToolStripMenuItem2;
	private: System::Windows::Forms::ToolStripMenuItem^  editToolStripMenuItem3;
	private: System::Windows::Forms::ToolStripMenuItem^  aboutToolStripMenuItem;
	private: System::Windows::Forms::ListBox^  animlist;

	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Button^  addanm;
	private: System::Windows::Forms::Button^  editanm;
	private: System::Windows::Forms::Button^  remanm;



	private: System::Windows::Forms::ListBox^  seqlist;


	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Button^  button4;
	private: System::Windows::Forms::Button^  button5;
	private: System::Windows::Forms::Button^  button6;
	private: System::Windows::Forms::Button^  add_to_seq;


	private:
		/// <summary>
		/// “ребуетс€ переменна€ конструктора.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// ќб€зательный метод дл€ поддержки конструктора - не измен€йте
		/// содержимое данного метода при помощи редактора кода.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
			this->menuStrip1 = (gcnew System::Windows::Forms::MenuStrip());
			this->fileToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->newToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->openToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->saveToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->saveAsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exitToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->exitToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->editToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->editToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->removeToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->removeToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->sequenceToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->editToolStripMenuItem2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->removeToolStripMenuItem2 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->editToolStripMenuItem3 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->helpToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->aboutToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->animlist = (gcnew System::Windows::Forms::ListBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->addanm = (gcnew System::Windows::Forms::Button());
			this->editanm = (gcnew System::Windows::Forms::Button());
			this->remanm = (gcnew System::Windows::Forms::Button());
			this->seqlist = (gcnew System::Windows::Forms::ListBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->button4 = (gcnew System::Windows::Forms::Button());
			this->button5 = (gcnew System::Windows::Forms::Button());
			this->button6 = (gcnew System::Windows::Forms::Button());
			this->add_to_seq = (gcnew System::Windows::Forms::Button());
			this->menuStrip1->SuspendLayout();
			this->SuspendLayout();
			// 
			// menuStrip1
			// 
			this->menuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(4) {this->fileToolStripMenuItem, 
				this->editToolStripMenuItem, this->sequenceToolStripMenuItem, this->helpToolStripMenuItem});
			this->menuStrip1->Location = System::Drawing::Point(0, 0);
			this->menuStrip1->Name = L"menuStrip1";
			this->menuStrip1->Size = System::Drawing::Size(464, 24);
			this->menuStrip1->TabIndex = 0;
			this->menuStrip1->Text = L"menuStrip1";
			// 
			// fileToolStripMenuItem
			// 
			this->fileToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(7) {this->newToolStripMenuItem, 
				this->toolStripMenuItem1, this->openToolStripMenuItem, this->saveToolStripMenuItem, this->saveAsToolStripMenuItem, this->exitToolStripMenuItem, 
				this->exitToolStripMenuItem1});
			this->fileToolStripMenuItem->Name = L"fileToolStripMenuItem";
			this->fileToolStripMenuItem->Size = System::Drawing::Size(37, 20);
			this->fileToolStripMenuItem->Text = L"&File";
			this->fileToolStripMenuItem->Click += gcnew System::EventHandler(this, &Form1::fileToolStripMenuItem_Click);
			// 
			// newToolStripMenuItem
			// 
			this->newToolStripMenuItem->Name = L"newToolStripMenuItem";
			this->newToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->newToolStripMenuItem->Text = L"&New";
			// 
			// toolStripMenuItem1
			// 
			this->toolStripMenuItem1->Name = L"toolStripMenuItem1";
			this->toolStripMenuItem1->Size = System::Drawing::Size(149, 6);
			// 
			// openToolStripMenuItem
			// 
			this->openToolStripMenuItem->Name = L"openToolStripMenuItem";
			this->openToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->openToolStripMenuItem->Text = L"&Open";
			// 
			// saveToolStripMenuItem
			// 
			this->saveToolStripMenuItem->Name = L"saveToolStripMenuItem";
			this->saveToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->saveToolStripMenuItem->Text = L"&Save";
			// 
			// saveAsToolStripMenuItem
			// 
			this->saveAsToolStripMenuItem->Name = L"saveAsToolStripMenuItem";
			this->saveAsToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->saveAsToolStripMenuItem->Text = L"&Save As";
			// 
			// exitToolStripMenuItem
			// 
			this->exitToolStripMenuItem->Name = L"exitToolStripMenuItem";
			this->exitToolStripMenuItem->Size = System::Drawing::Size(149, 6);
			// 
			// exitToolStripMenuItem1
			// 
			this->exitToolStripMenuItem1->Name = L"exitToolStripMenuItem1";
			this->exitToolStripMenuItem1->Size = System::Drawing::Size(152, 22);
			this->exitToolStripMenuItem1->Text = L"&Exit";
			// 
			// editToolStripMenuItem
			// 
			this->editToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {this->editToolStripMenuItem1, 
				this->removeToolStripMenuItem, this->removeToolStripMenuItem1});
			this->editToolStripMenuItem->Name = L"editToolStripMenuItem";
			this->editToolStripMenuItem->Size = System::Drawing::Size(75, 20);
			this->editToolStripMenuItem->Text = L"&Animation";
			// 
			// editToolStripMenuItem1
			// 
			this->editToolStripMenuItem1->Name = L"editToolStripMenuItem1";
			this->editToolStripMenuItem1->Size = System::Drawing::Size(117, 22);
			this->editToolStripMenuItem1->Text = L"&Edit";
			// 
			// removeToolStripMenuItem
			// 
			this->removeToolStripMenuItem->Name = L"removeToolStripMenuItem";
			this->removeToolStripMenuItem->Size = System::Drawing::Size(114, 6);
			this->removeToolStripMenuItem->Click += gcnew System::EventHandler(this, &Form1::removeToolStripMenuItem_Click);
			// 
			// removeToolStripMenuItem1
			// 
			this->removeToolStripMenuItem1->Name = L"removeToolStripMenuItem1";
			this->removeToolStripMenuItem1->Size = System::Drawing::Size(117, 22);
			this->removeToolStripMenuItem1->Text = L"&Remove";
			// 
			// sequenceToolStripMenuItem
			// 
			this->sequenceToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {this->editToolStripMenuItem2, 
				this->removeToolStripMenuItem2, this->editToolStripMenuItem3});
			this->sequenceToolStripMenuItem->Name = L"sequenceToolStripMenuItem";
			this->sequenceToolStripMenuItem->Size = System::Drawing::Size(70, 20);
			this->sequenceToolStripMenuItem->Text = L"&Sequence";
			// 
			// editToolStripMenuItem2
			// 
			this->editToolStripMenuItem2->Name = L"editToolStripMenuItem2";
			this->editToolStripMenuItem2->Size = System::Drawing::Size(94, 22);
			this->editToolStripMenuItem2->Text = L"Edit";
			// 
			// removeToolStripMenuItem2
			// 
			this->removeToolStripMenuItem2->Name = L"removeToolStripMenuItem2";
			this->removeToolStripMenuItem2->Size = System::Drawing::Size(91, 6);
			// 
			// editToolStripMenuItem3
			// 
			this->editToolStripMenuItem3->Name = L"editToolStripMenuItem3";
			this->editToolStripMenuItem3->Size = System::Drawing::Size(94, 22);
			this->editToolStripMenuItem3->Text = L"Edit";
			// 
			// helpToolStripMenuItem
			// 
			this->helpToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->aboutToolStripMenuItem});
			this->helpToolStripMenuItem->Name = L"helpToolStripMenuItem";
			this->helpToolStripMenuItem->Size = System::Drawing::Size(44, 20);
			this->helpToolStripMenuItem->Text = L"&Help";
			// 
			// aboutToolStripMenuItem
			// 
			this->aboutToolStripMenuItem->Name = L"aboutToolStripMenuItem";
			this->aboutToolStripMenuItem->Size = System::Drawing::Size(107, 22);
			this->aboutToolStripMenuItem->Text = L"&About";
			// 
			// animlist
			// 
			this->animlist->FormattingEnabled = true;
			this->animlist->Location = System::Drawing::Point(12, 53);
			this->animlist->Name = L"animlist";
			this->animlist->Size = System::Drawing::Size(119, 251);
			this->animlist->TabIndex = 1;
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(9, 35);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(58, 13);
			this->label1->TabIndex = 2;
			this->label1->Text = L"Animations";
			// 
			// addanm
			// 
			this->addanm->Location = System::Drawing::Point(15, 316);
			this->addanm->Name = L"addanm";
			this->addanm->Size = System::Drawing::Size(55, 24);
			this->addanm->TabIndex = 3;
			this->addanm->Text = L"Add";
			this->addanm->UseVisualStyleBackColor = true;
			// 
			// editanm
			// 
			this->editanm->Location = System::Drawing::Point(76, 316);
			this->editanm->Name = L"editanm";
			this->editanm->Size = System::Drawing::Size(55, 24);
			this->editanm->TabIndex = 4;
			this->editanm->Text = L"Edit";
			this->editanm->UseVisualStyleBackColor = true;
			// 
			// remanm
			// 
			this->remanm->Location = System::Drawing::Point(137, 83);
			this->remanm->Name = L"remanm";
			this->remanm->Size = System::Drawing::Size(160, 24);
			this->remanm->TabIndex = 5;
			this->remanm->Text = L"Remove";
			this->remanm->UseVisualStyleBackColor = true;
			// 
			// seqlist
			// 
			this->seqlist->FormattingEnabled = true;
			this->seqlist->Location = System::Drawing::Point(303, 54);
			this->seqlist->Name = L"seqlist";
			this->seqlist->Size = System::Drawing::Size(150, 251);
			this->seqlist->TabIndex = 6;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(300, 35);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(61, 13);
			this->label2->TabIndex = 7;
			this->label2->Text = L"Sequences";
			// 
			// button4
			// 
			this->button4->Location = System::Drawing::Point(303, 317);
			this->button4->Name = L"button4";
			this->button4->Size = System::Drawing::Size(40, 24);
			this->button4->TabIndex = 8;
			this->button4->Text = L"Add";
			this->button4->UseVisualStyleBackColor = true;
			// 
			// button5
			// 
			this->button5->Location = System::Drawing::Point(349, 317);
			this->button5->Name = L"button5";
			this->button5->Size = System::Drawing::Size(35, 24);
			this->button5->TabIndex = 9;
			this->button5->Text = L"Edit";
			this->button5->UseVisualStyleBackColor = true;
			// 
			// button6
			// 
			this->button6->Location = System::Drawing::Point(390, 317);
			this->button6->Name = L"button6";
			this->button6->Size = System::Drawing::Size(63, 24);
			this->button6->TabIndex = 10;
			this->button6->Text = L"Remove";
			this->button6->UseVisualStyleBackColor = true;
			// 
			// add_to_seq
			// 
			this->add_to_seq->Location = System::Drawing::Point(137, 54);
			this->add_to_seq->Name = L"add_to_seq";
			this->add_to_seq->Size = System::Drawing::Size(160, 23);
			this->add_to_seq->TabIndex = 11;
			this->add_to_seq->Text = L"Add to selected sequence";
			this->add_to_seq->UseVisualStyleBackColor = true;
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(464, 352);
			this->Controls->Add(this->add_to_seq);
			this->Controls->Add(this->button6);
			this->Controls->Add(this->button5);
			this->Controls->Add(this->button4);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->seqlist);
			this->Controls->Add(this->remanm);
			this->Controls->Add(this->editanm);
			this->Controls->Add(this->addanm);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->animlist);
			this->Controls->Add(this->menuStrip1);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->MainMenuStrip = this->menuStrip1;
			this->MaximizeBox = false;
			this->Name = L"Form1";
			this->Text = L"AnimCA Package Script editor";
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->menuStrip1->ResumeLayout(false);
			this->menuStrip1->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) {
			 }
	private: System::Void fileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
			 }
	private: System::Void removeToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
			 }
};
}

