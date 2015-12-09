#pragma once

namespace animcascriptmaker {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// ������ ��� AnimationEditDlg
	/// </summary>
	public ref class AnimationEditDlg : public System::Windows::Forms::Form
	{
	public:
		AnimationEditDlg(void)
		{
			InitializeComponent();
			//
			//TODO: �������� ��� ������������
			//
		}

	protected:
		/// <summary>
		/// ���������� ��� ������������ �������.
		/// </summary>
		~AnimationEditDlg()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  anim_name_label;
	protected: 

	private:
		/// <summary>
		/// ��������� ���������� ������������.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// ������������ ����� ��� ��������� ������������ - �� ���������
		/// ���������� ������� ������ ��� ������ ��������� ����.
		/// </summary>
		void InitializeComponent(void)
		{
			this->anim_name_label = (gcnew System::Windows::Forms::Label());
			this->SuspendLayout();
			// 
			// anim_name_label
			// 
			this->anim_name_label->AutoSize = true;
			this->anim_name_label->Location = System::Drawing::Point(12, 9);
			this->anim_name_label->Name = L"anim_name_label";
			this->anim_name_label->Size = System::Drawing::Size(58, 13);
			this->anim_name_label->TabIndex = 0;
			this->anim_name_label->Text = L"anim name";
			// 
			// AnimationEditDlg
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(444, 233);
			this->Controls->Add(this->anim_name_label);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"AnimationEditDlg";
			this->ShowIcon = false;
			this->ShowInTaskbar = false;
			this->Text = L"Animation properties";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	};
}
